package store

import (
	"bytes"
	"crypto/md5"
	"fmt"
	"io"
	"log"
	"os"
	"path/filepath"
	"strconv"
	"strings"

	"github.com/buaazp/zimg/conf"
)

const (
	OrigName = "0*0"
)

type LocalStore struct {
	Path string
}

func NewLocalStore(c conf.ServerConf) (*LocalStore, error) {
	abs, err := filepath.Abs(c.ImagePath)
	if err != nil {
		return nil, err
	}
	s := new(LocalStore)
	s.Path = abs
	return s, nil
}

func (s *LocalStore) Set(data []byte) (string, int64, error) {
	md5sum := fmt.Sprintf("%x", md5.Sum(data))
	dirPath, err := s.calcPath(md5sum)
	if err != nil {
		return "", 0, err
	}
	mtime, err := WriteData(dirPath, data)
	if err != nil {
		return "", mtime, err
	}
	return md5sum, mtime, nil
}

func (s *LocalStore) calcPath(md5sum string) (string, error) {
	level1, err := strconv.ParseUint(md5sum[:3], 16, 0)
	if err != nil {
		return "", err
	}
	level2, err := strconv.ParseUint(md5sum[3:6], 16, 0)
	if err != nil {
		return "", err
	}
	l1 := strconv.FormatUint(level1/4, 10)
	l2 := strconv.FormatUint(level2/4, 10)
	relativePath := filepath.Join(l1, l2, md5sum)
	absolutePath := filepath.Join(s.Path, relativePath)
	log.Printf("img path: %s", relativePath)
	return absolutePath, nil
}

func WriteData(dirPath string, data []byte) (int64, error) {
	err := os.MkdirAll(dirPath, 0755)
	if err != nil {
		return 0, err
	}
	origPath := filepath.Join(dirPath, OrigName)

	f, err := os.OpenFile(origPath, os.O_WRONLY|os.O_CREATE|os.O_TRUNC, 0664)
	if err != nil {
		return 0, err
	}
	defer f.Close()
	if n, err := f.Write(data); err != nil {
		return 0, err
	} else if n < len(data) {
		return 0, io.ErrShortWrite
	}
	stat, err := f.Stat()
	if err != nil {
		return 0, err
	}
	mtime := stat.ModTime().UnixNano()
	return mtime, err
}

func (s *LocalStore) Put(key string, data []byte) (int64, error) {
	parts := strings.Split(key, ",")
	if len(parts) == 3 {
		key = fmt.Sprintf("%s,%s", parts[0], parts[1])
	}

	dirPath, err := s.calcPath(key)
	if err != nil {
		return 0, err
	}

	return WriteData(dirPath, data)
}

func readAll(r io.Reader, capacity int64) (b []byte, err error) {
	buf := bytes.NewBuffer(make([]byte, 0, capacity))
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
	_, err = buf.ReadFrom(r)
	return buf.Bytes(), err
}

func (s *LocalStore) Get(key string) ([]byte, int64, error) {
	dirPath, err := s.calcPath(key)
	if err != nil {
		return nil, 0, err
	}
	origPath := filepath.Join(dirPath, OrigName)
	f, err := os.Open(origPath)
	if err != nil {
		return nil, 0, err
	}
	defer f.Close()
	stat, err := f.Stat()
	if err != nil {
		return nil, 0, err
	}
	mtime := stat.ModTime().UnixNano()
	var n int64
	if size := stat.Size(); size < 1e9 {
		n = size
	}
	data, err := readAll(f, n+bytes.MinRead)
	if err != nil {
		return nil, mtime, err
	}
	return data, mtime, nil
}

func (s *LocalStore) Del(key string) error {
	dirPath, err := s.calcPath(key)
	if err != nil {
		return err
	}
	return os.RemoveAll(dirPath)
}
