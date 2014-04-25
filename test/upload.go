package main

import (
	"bytes"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"mime/multipart"
	"net/http"
	"os"
)

// 获取大小的接口
type Sizer interface {
	Size() int64
}

// hello world, the web server
func HelloServer(w http.ResponseWriter, r *http.Request) {
	if "POST" == r.Method {
		for k, v := range r.Header {
			for _, vv := range v {
				fmt.Printf("k: %v v: %v\n", k, vv)
			}
		}

		b1, err := ioutil.ReadAll(r.Body)
		n1 := len(b1)
		if err != nil {
			http.Error(w, err.Error(), 500)
			return
		}
		fmt.Printf("%d bytes:\n%s\n", n1, string(b1))

		/*
			file, header, err := r.FormFile("filename")
			if err != nil {
				http.Error(w, err.Error(), 500)
				return
			}
			defer file.Close()

			f, err := os.Create("gosvr." + header.Filename)
			defer f.Close()
			io.Copy(f, file)
		*/

		w.Header().Add("Content-Type", "text/html")
		w.WriteHeader(200)
		//fmt.Fprintf(w, "上传文件的大小为: %d</br>\n", file.(Sizer).Size())
		fmt.Fprintf(w, "HTTP包体的大小为: %d</br>\n", n1)
		return
	}

	// 上传页面
	w.Header().Add("Content-Type", "text/html")
	w.WriteHeader(200)
	html := `
<form enctype="multipart/form-data" action="http://127.0.0.1:12345/" method="POST">
    Send this file: <input name="file" type="file" />
    <input type="submit" value="Send File" />
</form>
`
	io.WriteString(w, html)
}

func sendHandler(w http.ResponseWriter, r *http.Request) {
	Upload()
}

func main() {
	http.HandleFunc("/send", sendHandler)
	http.HandleFunc("/", HelloServer)
	err := http.ListenAndServe(":12345", nil)
	if err != nil {
		log.Fatal("ListenAndServe: ", err)
	}
}

func Upload() (err error) {
	// Create buffer
	buf := new(bytes.Buffer) // caveat IMO dont use this for large files, \
	// create a tmpfile and assemble your multipart from there (not tested)
	w := multipart.NewWriter(buf)

	upload_target := "http://127.0.0.1:12345/"
	filename := "5f189.jpeg"

	// Create file field
	fw, err := w.CreateFormFile("file", filename) //这里的file很重要，必须和服务器端的FormFile一致
	if err != nil {
		fmt.Println("c")
		return err
	}
	fd, err := os.Open(filename)
	if err != nil {
		fmt.Println("d")
		return err
	}

	defer fd.Close()
	// Write file field from file to upload
	_, err = io.Copy(fw, fd)
	if err != nil {
		fmt.Println("e")
		return err
	}
	// Important if you do not close the multipart writer you will not have a
	// terminating boundry
	w.Close()

	req, err := http.NewRequest("POST", upload_target, buf)
	if err != nil {
		fmt.Println("f")
		return err
	}
	req.Header.Set("Content-Type", w.FormDataContentType())
	var client http.Client
	res, err := client.Do(req)
	if err != nil {
		fmt.Println("g")
		return err
	}
	io.Copy(os.Stderr, res.Body) // Replace this with Status.Code check
	fmt.Println("h")

	return err
}
