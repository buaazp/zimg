package server

import (
	"bytes"
	"crypto/sha1"
	_ "expvar"
	"fmt"
	"io"
	"log"
	"net"
	"net/http"
	httpprof "net/http/pprof"
	"strconv"
	"strings"
	"time"

	"github.com/buaazp/zimg/conf"
	"github.com/buaazp/zimg/store"
	"github.com/buaazp/zimg/util"
	"github.com/gorilla/mux"
	"gopkg.in/yaml.v2"
)

type Server struct {
	http.RoundTripper
	c conf.ServerConf
	r *mux.Router

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

	s := &Server{}
	s.storage = storage
	s.c = c
	DefaultOutputFormat = c.Format
	DefaultCompressionQuality = c.Quality
	LongImageSideRatio = c.LongImageRatio
	LongImageSideRatioInverse = 1 / LongImageSideRatio
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
	s.r.HandleFunc(`/obj`, s.apiUploadImage).Methods("POST")
	sub := s.r.Path(`/obj/{key}`).Subrouter()
	sub.Methods("PUT").HandlerFunc(s.apiUpdateImage)
	sub.Methods("HEAD", "GET").HandlerFunc(s.apiGetImage)
	sub.Methods("DELETE").HandlerFunc(s.apiDelImage)
	s.r.HandleFunc(`/info/{key}`, s.apiInfoImage).Methods("GET")

	// for config
	s.r.HandleFunc(`/conf`, s.apiGetConf).Methods("GET")

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

func (s *Server) readAndDetect(r io.Reader) (b []byte, ctype string, err error, code int) {
	// If the buffer overflows, we will get bytes.ErrTooLarge.
	// Return that as an error. Any other panic remains.
	defer func() {
		e := recover()
		if e == nil {
			return
		}
		if panicErr, ok := e.(error); ok && panicErr == bytes.ErrTooLarge {
			err = panicErr
		} else {
			panic(e)
		}
	}()

	p := make([]byte, bytes.MinRead)
	_, e := r.Read(p)
	if e != nil && e != io.EOF {
		err = e
		code = 500
		return
	}

	ctype = http.DetectContentType(p)
	if !s.AllowedType(ctype) {
		err = ErrNotAllowed
		code = http.StatusUnsupportedMediaType
		return
	}

	buf := bytes.NewBuffer(p)
	_, err = buf.ReadFrom(r)
	if err != nil {
		code = 500
		return
	}
	return buf.Bytes(), ctype, nil, 200
}

func (s *Server) readUploadBody(r *http.Request) ([]byte, string, error, int) {
	if r.ContentLength > s.c.MaxSize {
		return nil, "", ErrObjectTooLarge, http.StatusRequestEntityTooLarge
	}

	var rd io.Reader
	if strings.Contains(r.Header.Get("Content-Type"), "multipart/form-data") {
		// multipart form upload
		err := r.ParseMultipartForm(s.c.MaxSize)
		if err != nil {
			return nil, "", err, 500
		}
		file, _, err := r.FormFile(s.c.MultipartKey)
		if err != nil {
			return nil, "", err, 500
		}
		rd = file
	} else {
		// raw-post upload
		rd = io.LimitReader(r.Body, s.c.MaxSize)
	}
	body, ctype, err, code := s.readAndDetect(rd)
	if err != nil {
		return nil, "", err, code
	}
	if int64(len(body)) > s.c.MaxSize {
		return nil, "", ErrObjectTooLarge, http.StatusRequestEntityTooLarge
	}
	return body, ctype, nil, 200
}

func (s *Server) apiUploadImage(w http.ResponseWriter, r *http.Request) {
	data, ctype, err, code := s.readUploadBody(r)
	if err != nil {
		util.APIResponse(w, code, err)
		log.Printf("%s upload fail %d - - %s", r.RemoteAddr, code, err)
		return
	}

	var imgResult ImageResult
	if format := util.GetFileType(ctype); format == "images" {
		// image type files
		imgResult, err = ImagickInfo(data)
		if err != nil {
			util.APIResponse(w, 400, err)
			return
		}
	} else {
		// other type files
		imgResult.Format = format
	}

	key, mtime, err := s.storage.Set(data)
	if err != nil {
		util.APIResponse(w, 500, err)
		log.Printf("%s upload fail 500 %s %d %s", r.RemoteAddr, key, len(data), err)
		return
	}
	imgResult.Key = key
	imgResult.MTime = mtime

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
			return true
		}
	}
	w.Header().Set("ETag", sha1sum)
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

	origin, mtime, err := s.storage.Get(key)
	if err != nil {
		util.APIResponse(w, 500, err)
		log.Printf("%s get fail 500 %s - %s", r.RemoteAddr, key, err)
		return
	}

	var data []byte
	ctype := http.DetectContentType(origin)
	format := util.GetFileType(ctype)
	if format == "images" && cp != nil {
		var code int
		format, data, err, code = Convert(origin, cp)
		if err != nil {
			util.APIResponse(w, code, err)
			log.Printf("%s get fail %d %s - %s", r.RemoteAddr, code, key, err)
			return
		}
	} else {
		data = origin
	}

	for k, v := range s.c.Headers {
		w.Header().Set(k, v)
	}
	if s.c.MaxAge > 0 {
		w.Header().Set("Cache-Control", fmt.Sprintf("max-age=%d", s.c.MaxAge))
	}
	w.Header().Set("Content-Type", ctype)
	w.Header().Set("Content-Length", strconv.Itoa(len(data)))
	if r.Method == "HEAD" {
		w.WriteHeader(200)
		log.Printf("%s head succ 200 %s - -", r.RemoteAddr, key)
		return
	}

	sha1sum := fmt.Sprintf("%x", sha1.Sum(data))
	done := checkAndSetEtag(w, r, sha1sum)
	if done {
		w.WriteHeader(304)
		log.Printf("%s down succ 304 %s - -", r.RemoteAddr, key)
		return
	}

	reader := util.NewByteReadSeeker(data)
	http.ServeContent(w, r, "", util.CTime(mtime), reader)
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

	var imgResult ImageResult
	imgResult.Key = key
	origin, mtime, err := s.storage.Get(key)
	if err != nil {
		util.APIResponse(w, 500, err)
		log.Printf("%s info fail 500 %s - %s", r.RemoteAddr, key, err)
		return
	}
	imgResult.MTime = mtime

	ctype := http.DetectContentType(origin)
	if format := util.GetFileType(ctype); format == "images" {
		// image type files
		imgResult, err = ImagickInfo(origin)
		if err != nil {
			util.APIResponse(w, 500, err)
			return
		}
	} else {
		// other type files
		imgResult.Format = format
	}

	log.Printf("%s info succ 200 %s - -", r.RemoteAddr, key)
	util.APIResponse(w, 200, imgResult)
}

func (s *Server) apiGetConf(w http.ResponseWriter, r *http.Request) {
	data, err := yaml.Marshal(s.c)
	if err != nil {
		util.APIResponse(w, 500, fmt.Errorf("yaml encode: %s", err))
		return
	}
	log.Printf("%s get succ 200 /conf - -", r.RemoteAddr)
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
