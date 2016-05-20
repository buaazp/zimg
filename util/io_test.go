package util

import (
	"bytes"
	"io"
	"io/ioutil"
	"math/rand"
	"os"
	"path/filepath"
	"reflect"
	"runtime"
	"testing"
	"time"
)

func equal(t *testing.T, act, exp interface{}) {
	if !reflect.DeepEqual(exp, act) {
		_, file, line, _ := runtime.Caller(1)
		t.Fatalf("\n\033[31m%s:%d:\n\texp: %#v\n\tgot: %#v\033[39m\n\n",
			filepath.Base(file), line, exp, act)
	}
}

func init() {
	rand.Seed(time.Now().Unix())
}

func TestByteReadSeeker(t *testing.T) {
	b := []byte("1234567890")
	buf1 := make([]byte, 4)
	buf2 := make([]byte, 7)

	r := NewByteReadSeeker(b)
	n, err := r.Read(buf1)
	if n != len(buf1) || err != nil {
		t.Fatal("n != len(buf1) || err != nil")
	}
	equal(t, buf1, []byte("1234"))
	n, err = r.Read(buf2)
	if n != 6 || err != io.EOF {
		t.Fatal("n != 6 || err != io.EOF")
	}
	equal(t, buf2[:6], []byte("567890"))

	n, err = r.Read(buf2)
	if n != 0 || err != io.EOF {
		t.Fatal("n != 0 || err != io.EOF")
	}

	r.Seek(1, os.SEEK_SET)
	r.Read(buf1)
	equal(t, buf1, []byte("2345"))

	r.Seek(0, os.SEEK_SET)
	r.Seek(2, os.SEEK_CUR)
	r.Read(buf1)
	equal(t, buf1, []byte("3456"))

	r.Seek(-4, os.SEEK_END)
	r.Read(buf1)
	equal(t, buf1, []byte("7890"))

	r.Seek(0, os.SEEK_SET)
	off, err := r.Seek(11, os.SEEK_SET)
	if off != 0 || err == nil {
		t.Fatal("off != 0 || err == nil")
	}
}

func TestSniffReader(t *testing.T) {
	b := []byte("1234567890")
	r := NewSniffReader(bytes.NewBuffer(b), 3)
	ioutil.ReadAll(r)
	equal(t, r.Bytes(), []byte("123"))
}
