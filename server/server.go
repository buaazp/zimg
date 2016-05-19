package server

import (
	"crypto/sha1"
	_ "expvar"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"net"
	"net/http"
	httpprof "net/http/pprof"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/buaazp/zimg/conf"
	"github.com/buaazp/zimg/store"
	"github.com/buaazp/zimg/util"
	"github.com/gorilla/mux"
	"gopkg.in/yaml.v2"
)

type Server struct {
	mu   sync.RWMutex
	exit chan int

	RoundTripper http.RoundTripper
	c            conf.ServerConf
	r            *mux.Router

	storage store.Storage
}

func NewServer(c conf.ServerConf) (*Server, error) {
	var storage store.Storage
	var err error
	switch c.StorageMode {
	case store.S_LOCAL:
		storage, err = store.NewLocalStore(c)
	default:
		return nil, fmt.Errorf("unknow storage mode: %s", c.StorageMode)
	}
	if err != nil {
		return nil, err
	}

	DefaultOutputFormat = c.Format
	DefaultCompressionQuality = c.Quality

	s := &Server{}
	s.storage = storage
	s.c = c
	s.RoundTripper = &http.Transport{
		Proxy: http.ProxyFromEnvironment,
		Dial: (&net.Dialer{
			Timeout:   c.Timeout.Duration,
			KeepAlive: 30 * time.Second,
		}).Dial,
		ResponseHeaderTimeout: c.Timeout.Duration,
	}

	s.r = mux.NewRouter()
	// for images
	s.r.HandleFunc(`/images`, s.apiUploadImage).Methods("POST")
	sub := s.r.Path(`/images/{key}`).Subrouter()
	sub.Methods("PUT").HandlerFunc(s.apiUpdateImage)
	sub.Methods("HEAD", "GET").HandlerFunc(s.apiGetImage)
	sub.Methods("DELETE").HandlerFunc(s.apiDelImage)
	s.r.HandleFunc(`/info/{key}`, s.apiInfoImage).Methods("GET")

	// for config
	s.r.HandleFunc(`/conf`, s.apiGetConf).Methods("GET")
	s.r.HandleFunc(`/conf`, s.apiReloadConf).Methods("POST")

	// for debugging
	s.r.HandleFunc(`/debug/pprof/cmdline`, httpprof.Cmdline)
	s.r.HandleFunc(`/debug/pprof/profile`, httpprof.Profile)
	s.r.HandleFunc(`/debug/pprof/symbol`, httpprof.Symbol)
	s.r.PathPrefix(`/debug/pprof/`).HandlerFunc(httpprof.Index)
	s.r.Handle("/debug/vars", http.DefaultServeMux)

	// for htmls
	s.r.PathPrefix(`/`).Handler(http.FileServer(http.Dir("./www/"))).Methods("GET")

	s.r.NotFoundHandler = http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		util.APIResponse(w, 400, "zimg api uri error")
	})
	return s, nil
}

func (s *Server) readUploadBody(r *http.Request) ([]byte, error) {
	if r.ContentLength > s.c.MaxSize {
		return nil, ErrObjectTooLarge
	}

	var (
		body []byte
		err  error
	)
	ctype := r.Header.Get("Content-Type")
	if strings.Contains(ctype, "multipart/form-data") {
		// multipart form upload
		err := r.ParseMultipartForm(s.c.MaxSize)
		if err != nil {
			return nil, err
		}
		file, _, err := r.FormFile(s.c.MultipartKey)
		if err != nil {
			return nil, err
		}
		body, err = ioutil.ReadAll(file)
	} else {
		// raw-post upload
		body, err = ioutil.ReadAll(io.LimitReader(r.Body, s.c.MaxSize))
	}
	if err != nil && err != io.EOF {
		return nil, err
	}
	if int64(len(body)) >= s.c.MaxSize {
		return nil, ErrObjectTooLarge
	}
	return body, nil
}

func (s *Server) AllowedType(ctype string) bool {
	if len(s.c.AllowedTypes) == 0 ||
		s.c.AllowedTypes[0] == "all" {
		return true
	}
	if ctype == "" {
		return false
	}
	for _, t := range s.c.AllowedTypes {
		if strings.Contains(ctype, t) {
			return true
		}
	}
	return false
}

func (s *Server) apiUploadImage(w http.ResponseWriter, r *http.Request) {
	data, err := s.readUploadBody(r)
	if err != nil {
		code := 500
		if err == ErrObjectTooLarge {
			code = http.StatusRequestEntityTooLarge
		}
		util.APIResponse(w, code, err)
		log.Printf("%s upload fail %d - - %s", r.RemoteAddr, code, err)
		return
	}

	// TODO: detect http content type only read the head part
	btype := http.DetectContentType(data)
	if !s.AllowedType(btype) {
		util.APIResponse(w, 400, ErrNotAllowed)
		log.Printf("%s upload fail 400 - - %s", r.RemoteAddr, fmt.Sprintf("type [%s] not allowed", btype))
		return
	}

	imgResult, err := ImagickInfo(data)
	if err != nil {
		util.APIResponse(w, 400, err)
		return
	}

	key, err := s.storage.Set(data)
	if err != nil {
		util.APIResponse(w, 500, err)
		log.Printf("%s upload fail 500 %s %d %s", r.RemoteAddr, key, len(data), err)
		return
	}
	imgResult.Key = key
	imgResult.MTime = time.Now().UnixNano()

	util.APIResponse(w, 200, imgResult)
	log.Printf("%s upload succ 200 %s %d -", r.RemoteAddr, key, len(data))
	return
}

func checkAndSetEtag(w http.ResponseWriter, r *http.Request, sha1sum string) bool {
	if inm := r.Header.Get("If-None-Match"); inm != "" {
		if inm == sha1sum || inm == "*" {
			h := w.Header()
			delete(h, "Content-Type")
			delete(h, "Content-Length")
			w.WriteHeader(304)
			return true
		}
	}
	w.Header().Set("ETag", sha1sum)
	w.WriteHeader(200)
	return false
}

func (s *Server) apiUpdateImage(w http.ResponseWriter, r *http.Request) {
	key, _, err := parseKey(r)
	if err != nil {
		util.APIResponse(w, 400, err)
		log.Printf("%s put fail 400 - - %s", r.RemoteAddr, err)
		return
	}
	log.Printf("%s put succ 200 %s - -", r.RemoteAddr, key)
	return
}

func (s *Server) apiGetImage(w http.ResponseWriter, r *http.Request) {
	key, ext, err := parseKey(r)
	if err != nil {
		util.APIResponse(w, 400, err)
		log.Printf("%s get fail 400 - - %s", r.RemoteAddr, err)
		return
	}

	cp, err := GetConvertParam(r, key, ext)
	if err != nil {
		util.APIResponse(w, 400, err)
		log.Printf("%s get fail 400 %s - %s", r.RemoteAddr, key, err)
		return
	}

	origin, err := s.storage.Get(key)
	if err != nil {
		util.APIResponse(w, 500, err)
		log.Printf("%s get fail 500 %s - %s", r.RemoteAddr, key, err)
		return
	}

	var (
		data  []byte
		ctype string
	)
	if cp != nil {
		key = cp.Key
		var (
			format string
			code   int
		)
		format, data, err, code = Convert(origin, cp)
		if err != nil {
			util.APIResponse(w, code, err)
			log.Printf("%s get fail %d %s - %s", r.RemoteAddr, code, key, err)
			return
		}
		ctype = util.GetMimeType(format)
	} else {
		data = origin
		ctype = http.DetectContentType(data)
	}

	for k, v := range s.c.Headers {
		w.Header().Set(k, v)
	}
	w.Header().Set("Content-Type", ctype)
	w.Header().Set("Content-Length", strconv.Itoa(len(data)))
	if r.Method == "HEAD" {
		w.WriteHeader(200)
		log.Printf("%s head succ 200 %s - -", r.RemoteAddr, key)
		return
	}

	if s.c.MaxAge > 0 {
		w.Header().Set("Cache-Control", fmt.Sprintf("max-age=%d", s.c.MaxAge))
	}
	sha1sum := fmt.Sprintf("%x", sha1.Sum(data))
	done := checkAndSetEtag(w, r, sha1sum)
	if done {
		log.Printf("%s down succ 304 %s - -", r.RemoteAddr, key)
		return
	}
	// TODO: support http bytes range
	w.Write(data)
	log.Printf("%s down succ 200 %s %d -", r.RemoteAddr, key, len(data))
	return
}

func (s *Server) apiDelImage(w http.ResponseWriter, r *http.Request) {
	key, _, err := parseKey(r)
	if err != nil {
		util.APIResponse(w, 400, err)
		log.Printf("%s delete error 400 %s - %s", r.RemoteAddr, key, err)
		return
	}

	err = s.storage.Del(key)
	if err != nil {
		util.APIResponse(w, 500, err)
		log.Printf("%s delete error 500 %s - %s", r.RemoteAddr, key, err)
		return
	}
	util.APIResponse(w, 200, fmt.Sprintf("%s delete succ", key))
	log.Printf("%s delete succ 200 %s - -", r.RemoteAddr, key)
	return
}

func (s *Server) apiInfoImage(w http.ResponseWriter, r *http.Request) {
	key, _, err := parseKey(r)
	if err != nil {
		util.APIResponse(w, 400, err)
		log.Printf("%s info fail 400 - - %s", r.RemoteAddr, err)
		return
	}
	log.Printf("%s info succ 200 %s - -", r.RemoteAddr, key)
	util.APIResponse(w, 400, ErrNotImplement)
}

func (s *Server) apiGetConf(w http.ResponseWriter, r *http.Request) {
	s.mu.RLock()
	data, err := yaml.Marshal(s.c)
	s.mu.RUnlock()
	if err != nil {
		util.APIResponse(w, 500, fmt.Errorf("yaml encode: %s", err))
		return
	}
	log.Printf("%s get succ 200 /conf - -", r.RemoteAddr)
	util.APIResponse(w, 200, data)
}

func (s *Server) apiReloadConf(w http.ResponseWriter, r *http.Request) {
	cp := r.FormValue("conf_path")
	if cp == "" {
		cp = s.c.ConfPath
	}
	c, err := conf.LoadServerConf(cp)
	if err != nil {
		util.APIResponse(w, 500, fmt.Errorf("load conf err: %s", err))
		return
	}
	s.mu.Lock()
	s.c = c
	s.mu.Unlock()
	data, err := yaml.Marshal(c)
	if err != nil {
		util.APIResponse(w, 500, fmt.Errorf("yaml encode err: %s", err))
		return
	}
	log.Printf("%s post succ 200 /conf - -", r.RemoteAddr)
	util.APIResponse(w, 200, data)
}

func (s *Server) Serve() error {
	log.Printf("serving at %s\n", s.c.Host)
	err := http.ListenAndServe(s.c.Host, s.r)
	if err != nil {
		fmt.Println(err)
		fmt.Println("😭\nplease try again!")
	}
	return err
}

func (s *Server) Close() {
	log.Printf("Server closing...")
	fmt.Println("😝\nbyebye!")
}
