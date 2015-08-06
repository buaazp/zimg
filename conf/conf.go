package conf

import (
	"encoding/json"
	"io/ioutil"
)

type ServerConf struct {
	StorageMode string   `json:"mode"`
	ImagePath   string   `json:"img_path"`
	Backends    []string `json:"backends"`
	LogFile     string   `json:"log_path"`
	Timeout     int      `json:"timeout"`
}

var DefaultServerConf = ServerConf{
	StorageMode: "local",
	ImagePath:   "./img",
	Backends:    []string{"127.0.0.1:8888", "127.0.0.1:8889"},
	LogFile:     "./log/zimg.log",
	Timeout:     5,
}

func LoadServerConf(f string) (ServerConf, error) {
	data, err := ioutil.ReadFile(f)
	if err != nil {
		return DefaultServerConf, err
	}

	c := DefaultServerConf

	err = json.Unmarshal(data, &c)
	if err != nil {
		return DefaultServerConf, err
	}

	return c, nil
}
