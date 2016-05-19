package util

import (
	"encoding/json"
	"fmt"
	"net/http"
	"net/url"
	"strconv"
	"strings"
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

const (
	// CONNECT HTTP method
	CONNECT = "CONNECT"
	// DELETE HTTP method
	DELETE = "DELETE"
	// GET HTTP method
	GET = "GET"
	// HEAD HTTP method
	HEAD = "HEAD"
	// OPTIONS HTTP method
	OPTIONS = "OPTIONS"
	// PATCH HTTP method
	PATCH = "PATCH"
	// POST HTTP method
	POST = "POST"
	// PUT HTTP method
	PUT = "PUT"
	// TRACE HTTP method
	TRACE = "TRACE"
	//-------------
	// Media types
	//-------------
	ApplicationJSON                  = "application/json"
	ApplicationJSONCharsetUTF8       = ApplicationJSON + "; " + CharsetUTF8
	ApplicationJavaScript            = "application/javascript"
	ApplicationJavaScriptCharsetUTF8 = ApplicationJavaScript + "; " + CharsetUTF8
	ApplicationXML                   = "application/xml"
	ApplicationXMLCharsetUTF8        = ApplicationXML + "; " + CharsetUTF8
	ApplicationForm                  = "application/x-www-form-urlencoded"
	ApplicationProtobuf              = "application/protobuf"
	ApplicationMsgpack               = "application/msgpack"
	TextHTML                         = "text/html"
	TextHTMLCharsetUTF8              = TextHTML + "; " + CharsetUTF8
	TextPlain                        = "text/plain"
	TextPlainCharsetUTF8             = TextPlain + "; " + CharsetUTF8
	MultipartForm                    = "multipart/form-data"
	//---------
	// Charset
	//---------
	CharsetUTF8 = "charset=utf-8"
	//---------
	// Headers
	//---------
	AcceptEncoding     = "Accept-Encoding"
	Authorization      = "Authorization"
	ContentDisposition = "Content-Disposition"
	ContentEncoding    = "Content-Encoding"
	ContentLength      = "Content-Length"
	ContentType        = "Content-Type"
	Location           = "Location"
	Upgrade            = "Upgrade"
	Vary               = "Vary"
	WWWAuthenticate    = "WWW-Authenticate"
	XForwardedFor      = "X-Forwarded-For"
	XRealIP            = "X-Real-IP"
	//-----------
	// Protocols
	//-----------
	WebSocket = "websocket"

	indexFile = "index.html"

	NotFoundStr = "NOT FOUND"
)

var (
	NotFoundBytes  = []byte("NOT FOUND")
	NotFoundStrLen = strconv.Itoa(len(NotFoundStr))

	mimeTypesLower = map[string]string{
		"bmp":  "image/bmp",
		"bmp2": "image/bmp",
		"bmp3": "image/bmp",
		"css":  "text/css; charset=utf-8",
		"gif":  "image/gif",
		"htm":  "text/html; charset=utf-8",
		"html": "text/html; charset=utf-8",
		"jpe":  "image/jpeg",
		"jpg":  "image/jpeg",
		"jpeg": "image/jpeg",
		"js":   "application/x-javascript",
		"pdf":  "application/pdf",
		"png":  "image/png",
		"tiff": "image/tiff",
		"webp": "image/webp",
		"xml":  "text/xml; charset=utf-8",
	}

	mimeTypesReverse = map[string]string{
		"image/jpeg": "JPEG",
		"image/png":  "PNG",
		"image/webp": "WEBP",
		"image/gif":  "GIF",
		"image/bmp":  "BMP",
	}

	imageTypes = map[string]string{
		"jpg":  "JPEG",
		"jpe":  "JPEG",
		"jpeg": "JPEG",
		"webp": "WEBP",
		"png":  "PNG",
		"gif":  "GIF", // from others to gif is ignored now
	}
)

func GetMimeType(ext string) string {
	ext = strings.ToLower(ext)
	mtype, ok := mimeTypesLower[ext]
	if !ok {
		return "application/octet-stream"
	}
	return mtype
}

func FromMimeType(mime string) string {
	mime = strings.ToLower(mime)
	itype, ok := mimeTypesReverse[mime]
	if !ok {
		return ""
	}
	return itype
}

func GetImageType(ext string) string {
	ext = strings.ToLower(ext)
	ftype, ok := imageTypes[ext]
	if !ok {
		return ""
	}
	return ftype
}
