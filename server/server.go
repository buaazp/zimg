package server

import (
	"bytes"
	"crypto/sha1"
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

	"zimg/conf"
	"zimg/store"
	"zimg/util"

	"github.com/BurntSushi/toml"
	"github.com/gorilla/mux"
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
	s.r.HandleFunc(`/debug/pprof/cmdline`, httpprof.Cmdline)
	s.r.HandleFunc(`/debug/pprof/profile`, httpprof.Profile)
	s.r.HandleFunc(`/debug/pprof/symbol`, httpprof.Symbol)
	s.r.PathPrefix(`/debug/pprof/`).HandlerFunc(httpprof.Index)

	s.r.HandleFunc(`/images`, s.apiUploadImage).Methods("POST")
	sub := s.r.Path(`/images/{filename}`).Subrouter()
	sub.Methods("HEAD", "GET").HandlerFunc(s.apiGetImage)
	sub.Methods("DELETE").HandlerFunc(s.apiDelImage)
	s.r.HandleFunc(`/info/{filename}`, s.apiInfoImage).Methods("GET")
	s.r.HandleFunc(`/conf`, s.apiGetConf).Methods("GET")

	s.r.NotFoundHandler = http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		util.APIResponse(w, 400, "zimg api uri error")
	})
	return s, nil
}

type UploadResp struct {
	Key string `json:"key"`
}

func (s *Server) readUploadBody(r *http.Request) ([]byte, error) {
	if r.ContentLength > s.c.MaxSize {
		return nil, ErrObjectTooLarge
	}
	body, err := ioutil.ReadAll(io.LimitReader(r.Body, s.c.MaxSize))
	if err != nil && err != io.EOF {
		return nil, err
	}
	if int64(len(body)) >= s.c.MaxSize {
		return nil, ErrObjectTooLarge
	}
	return body, nil
}

func (s *Server) AllowedType(ctype string) bool {
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
	ctype := r.Header.Get("Content-Type")
	if !s.AllowedType(ctype) {
		util.APIResponse(w, 400, ErrNotAllowed)
		log.Printf("%s upload fail 400 - - %s", r.RemoteAddr, fmt.Sprintf("type [%s] not allowed", ctype))
		return
	}

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

	key, err := s.storage.Set(data)
	if err != nil {
		util.APIResponse(w, 500, err)
		log.Printf("%s upload fail 500 %s %d %s", r.RemoteAddr, key, len(data), err)
		return
	}

	var resp UploadResp
	resp.Key = key
	util.APIResponse(w, 200, resp)
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

func (s *Server) apiGetImage(w http.ResponseWriter, r *http.Request) {
	key := mux.Vars(r)["filename"]
	if key == "" {
		util.APIResponse(w, 400, ErrNoKey)
		log.Printf("%s head succ 400 %s - %s", r.RemoteAddr, key, ErrNoKey)
		return
	}

	data, err := s.storage.Get(key)
	if err != nil {
		util.APIResponse(w, 500, err)
		log.Printf("%s head succ 500 %s - %s", r.RemoteAddr, key, err)
		return
	}

	ctype := http.DetectContentType(data)
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

	sha1sum := fmt.Sprintf("%x", sha1.Sum(data))
	done := checkAndSetEtag(w, r, sha1sum)
	if done {
		log.Printf("%s down succ 304 %s - -", r.RemoteAddr, key)
		return
	}
	w.Write(data)
	log.Printf("%s down succ 200 %s %d -", r.RemoteAddr, key, len(data))
	return
}

func (s *Server) apiDelImage(w http.ResponseWriter, r *http.Request) {
	key := mux.Vars(r)["filename"]
	if key == "" {
		util.APIResponse(w, 400, ErrNoKey)
		log.Printf("%s delete error 400 %s - %s", r.RemoteAddr, key, ErrNoKey)
		return
	}

	err := s.storage.Del(key)
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
	log.Printf("info error: %s", ErrNotDone)
	util.APIResponse(w, 400, ErrNotDone)
}

func (s *Server) apiGetConf(w http.ResponseWriter, r *http.Request) {
	var buf bytes.Buffer
	enc := toml.NewEncoder(&buf)
	err := enc.Encode(s.c)
	if err != nil {
		util.APIResponse(w, 500, fmt.Errorf("toml encode: %s", err))
		return
	}
	util.APIResponse(w, 200, buf.Bytes())
}

func (s *Server) Serve() error {
	log.Printf("serving at %s\n", s.c.Host)
	err := http.ListenAndServe(s.c.Host, s.r)
	if err != nil {
		fmt.Println(err)
		fmt.Println("😭\nplz try again!")
	}
	return err
}

func (s *Server) Close() {
	log.Printf("Server closing...")
	fmt.Println("😝\nbyebye!")
}
