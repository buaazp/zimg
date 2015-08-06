package util

import (
	"encoding/json"
	"fmt"
	"net/http"
	"net/url"
	"strconv"
)

type APIMessage struct {
	Message string `json:"message"`
}

func APIResponse(w http.ResponseWriter, code int, data interface{}) {
	var m APIMessage
	var response []byte
	var err error
	var isJSON bool

	if 200 <= code && code < 300 {
		switch t := data.(type) {
		case string:
			m.Message = t
			response, _ = json.Marshal(m)
		case []byte:
			response = data.([]byte)
		case nil:
			response = []byte{}
		default:
			isJSON = true
			response, err = json.Marshal(data)
			if err != nil {
				code = 500
				data = err
			}
		}
	}

	if code >= 400 {
		isJSON = true
		m.Message = fmt.Sprintf(`%s`, data)
		response, _ = json.Marshal(m)
	}

	if isJSON {
		w.Header().Set("Content-Type", "application/json; charset=utf-8")
	}
	w.Header().Set("Content-Length", strconv.Itoa(len(response)))
	w.WriteHeader(code)
	w.Write(response)
}

func FormValues2Map(fv url.Values) map[string]string {
	ret := make(map[string]string)
	for k, v := range fv {
		ret[k] = v[0]
	}
	return ret
}

func CopyHeaders(dst, src http.Header) {
	for k := range dst {
		dst.Del(k)
	}
	for k, vs := range src {
		for _, v := range vs {
			dst.Add(k, v)
		}
	}
}
