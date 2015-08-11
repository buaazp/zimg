package conf

import (
	"time"

	"github.com/BurntSushi/toml"
)

type ServerConf struct {
	Host          string            `toml:"host"`
	Timeout       duration          `toml:"timeout"`
	LogFile       string            `toml:"log_file"`
	Debug         bool              `toml:"debug"`
	StorageMode   string            `toml:"mode"`
	SaveNew       bool              `toml:"save_new"`
	MaxSize       int64             `toml:"max_size"`
	AllowedTypes  []string          `toml:"allowed_types"`
	ImagePath     string            `toml:"img_path"`
	Backends      []string          `toml:"backends"`
	CacheHost     string            `toml:"cache_host"`
	Format        string            `toml:"default_format"`
	Quality       int               `toml:"default_quality"`
	DisableArgs   bool              `toml:"disable_args"`
	DisableType   bool              `toml:"disable_type"`
	DisableZoomUp bool              `toml:"disable_zoom_up"`
	ThumbsConf    string            `toml:"thumbs_conf"`
	Etag          bool              `toml:"etag"`
	Headers       map[string]string `toml:"headers"`
}

var DefaultServerConf = ServerConf{
	Host:          "0.0.0.0:4869",
	Timeout:       duration{5 * time.Second},
	LogFile:       "",
	Debug:         true,
	StorageMode:   "local",
	SaveNew:       true,
	MaxSize:       100 * 1024 * 1024,
	AllowedTypes:  []string{"jpeg", "jpg", "gif", "png", "webp"},
	ImagePath:     "./img",
	Backends:      []string{"127.0.0.1:22122", "127.0.0.1:6379"},
	CacheHost:     "127.0.0.1:11211",
	Format:        "jpeg",
	Quality:       75,
	DisableArgs:   false,
	DisableType:   false,
	DisableZoomUp: false,
	ThumbsConf:    "./conf/thumbs.toml",
	Etag:          true,
	Headers: map[string]string{
		"Server":        "zimg v4.0.0",
		"Cache-Control": "max-age=7776000",
	},
}

func LoadServerConf(f string) (ServerConf, error) {
	var c ServerConf
	if _, err := toml.DecodeFile(f, &c); err != nil {
		return DefaultServerConf, err
	}

	return c, nil
}

type duration struct {
	time.Duration
}

func (d duration) MarshalText() ([]byte, error) {
	return []byte(d.Duration.String()), nil
}

func (d *duration) UnmarshalText(text []byte) error {
	var err error
	d.Duration, err = time.ParseDuration(string(text))
	return err
}
