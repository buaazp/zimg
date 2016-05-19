package store

import (
	"crypto/md5"
	"fmt"
	"io/ioutil"
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

func (s *LocalStore) Set(data []byte) (string, error) {
	md5sum := fmt.Sprintf("%x", md5.Sum(data))
	dirPath, err := s.calcPath(md5sum)
	if err != nil {
		return "", err
	}
	err = WriteImage(dirPath, data)
	if err != nil {
		return "", err
	}
	return md5sum, nil
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

func WriteImage(dirPath string, data []byte) error {
	err := os.MkdirAll(dirPath, 0755)
	if err != nil {
		return err
	}
	origPath := filepath.Join(dirPath, OrigName)
	err = ioutil.WriteFile(origPath, data, 0664)
	if err != nil {
		return err
	}
	return nil
}

func (s *LocalStore) Put(key string, data []byte) error {
	parts := strings.Split(key, ",")
	if len(parts) == 3 {
		key = fmt.Sprintf("%s,%s", parts[0], parts[1])
	}

	dirPath, err := s.calcPath(key)
	if err != nil {
		return err
	}

	err = WriteImage(dirPath, data)
	if err != nil {
		return err
	}
	return nil
}

func (s *LocalStore) Get(key string) ([]byte, error) {
	dirPath, err := s.calcPath(key)
	if err != nil {
		return nil, err
	}
	origPath := filepath.Join(dirPath, OrigName)
	data, err := ioutil.ReadFile(origPath)
	if err != nil {
		return nil, err
	}
	return data, nil
}

func (s *LocalStore) Del(key string) error {
	dirPath, err := s.calcPath(key)
	if err != nil {
		return err
	}
	return os.RemoveAll(dirPath)
}
