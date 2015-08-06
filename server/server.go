package server

import (
	"fmt"
	"log"
	"net"
	"net/http"
	httpprof "net/http/pprof"
	"net/url"
	"sync"
	"time"

	"zimg/conf"
	"zimg/util"

	"github.com/gorilla/mux"
)

var (
	ErrNotDone = fmt.Errorf("this function is not come true")
)

type Server struct {
	mu   sync.RWMutex
	exit chan int

	RoundTripper http.RoundTripper
	u            url.URL
	c            conf.ServerConf
	r            *mux.Router
}

func NewServer(addr string, c conf.ServerConf) *Server {
	s := &Server{}
	s.u = url.URL{Scheme: "http", Host: addr}
	s.c = c
	s.RoundTripper = &http.Transport{
		Proxy: http.ProxyFromEnvironment,
		Dial: (&net.Dialer{
			Timeout:   time.Duration(c.Timeout) * time.Second,
			KeepAlive: 30 * time.Second,
		}).Dial,
		ResponseHeaderTimeout: time.Duration(c.Timeout) * time.Second,
	}

	s.r = mux.NewRouter()
	s.r.HandleFunc(`/debug/pprof/cmdline`, httpprof.Cmdline)
	s.r.HandleFunc(`/debug/pprof/profile`, httpprof.Profile)
	s.r.HandleFunc(`/debug/pprof/symbol`, httpprof.Symbol)
	s.r.PathPrefix(`/debug/pprof/`).HandlerFunc(httpprof.Index)

	s.r.HandleFunc(`/images`, s.apiUploadImage).Methods("POST")
	sub := s.r.Path(`/images/{param:\w+(,\w+)+}`).Subrouter()
	sub.Methods("DELETE").HandlerFunc(s.apiDelImage)
	sub.Methods("HEAD", "GET").HandlerFunc(s.apiGetImage)
	s.r.HandleFunc(`/info/{param:\w+(,\w+)+}`, s.apiInfoImage).Methods("GET")

	s.r.NotFoundHandler = http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		util.APIResponse(w, 400, "zimg api uri error")
	})
	return s
}

func (s *Server) apiUploadImage(w http.ResponseWriter, r *http.Request) {
	log.Printf("upload error: %s", ErrNotDone)
	util.APIResponse(w, 400, ErrNotDone)
}

func (s *Server) apiGetImage(w http.ResponseWriter, r *http.Request) {
	log.Printf("get error: %s", ErrNotDone)
	util.APIResponse(w, 400, ErrNotDone)
}

func (s *Server) apiDelImage(w http.ResponseWriter, r *http.Request) {
	log.Printf("del error: %s", ErrNotDone)
	util.APIResponse(w, 400, ErrNotDone)
}

func (s *Server) apiInfoImage(w http.ResponseWriter, r *http.Request) {
	log.Printf("info error: %s", ErrNotDone)
	util.APIResponse(w, 400, ErrNotDone)
}

func (s *Server) Serve() error {
	log.Printf("serving at %s\n", s.u.Host)
	return http.ListenAndServe(s.u.Host, s.r)
}

func (s *Server) Close() {
	log.Printf("Server closing...")
	fmt.Println("😝\nbyebye!")
}
