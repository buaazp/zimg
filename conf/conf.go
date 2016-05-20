package conf

import (
	"io/ioutil"
	"time"

	"github.com/buaazp/zimg/common"
	"gopkg.in/yaml.v2"
)

type ServerConf struct {
	Host           string            `yaml:"host"`
	Timeout        duration          `yaml:"timeout"`
	LogFile        string            `yaml:"log_file"`
	Debug          bool              `yaml:"debug"`
	StorageMode    string            `yaml:"mode"`
	SaveNew        bool              `yaml:"save_new"`
	MultipartKey   string            `yaml:"multipart_key"`
	MaxSize        int64             `yaml:"max_size"`
	AllowedTypes   []string          `yaml:"allowed_types"`
	ImagePath      string            `yaml:"img_path"`
	Backends       []string          `yaml:"backends"`
	CacheHost      string            `yaml:"cache_host"`
	Format         string            `yaml:"default_format"`
	Quality        uint              `yaml:"default_quality"`
	LongImageRatio float64           `yaml:"long_image_ratio"`
	DisableArgs    bool              `yaml:"disable_args"`
	DisableType    bool              `yaml:"disable_type"`
	DisableZoomUp  bool              `yaml:"disable_zoom_up"`
	Etag           bool              `yaml:"etag"`
	MaxAge         int64             `yaml:"max_age"`
	Headers        map[string]string `yaml:"headers"`
	Thumbs         []common.Thumb    `yaml:"thumbs"`
}

var DefaultServerConf = ServerConf{
	Host:           "0.0.0.0:4869",
	Timeout:        duration{5 * time.Second},
	LogFile:        "",
	Debug:          true,
	StorageMode:    "local",
	SaveNew:        true,
	MultipartKey:   "uploadfile",
	MaxSize:        100 * 1024 * 1024, // 100MB
	AllowedTypes:   []string{"jpeg", "jpg", "gif", "png", "webp"},
	ImagePath:      "./img",
	Backends:       []string{"127.0.0.1:22122", "127.0.0.1:6379"},
	CacheHost:      "127.0.0.1:11211",
	Format:         "jpeg",
	Quality:        75,
	LongImageRatio: 2.5,
	DisableArgs:    false,
	DisableType:    false,
	DisableZoomUp:  false,
	Etag:           true,
	MaxAge:         3600 * 24 * 90, // 90 days
	Headers: map[string]string{
		"Server":        "zimg v4.0.0",
		"Cache-Control": "max-age=7776000",
	},
	Thumbs: []common.Thumb{
		common.Thumb{
			Name:   "1080p",
			ID:     1,
			Mode:   common.ModeFit,
			Width:  1920,
			Height: 1080,
		},
		common.Thumb{
			Name:   "720p",
			ID:     2,
			Mode:   common.ModeFit,
			Width:  1280,
			Height: 720,
		},
		common.Thumb{
			Name:   "480p",
			ID:     3,
			Mode:   common.ModeFit,
			Width:  854,
			Height: 480,
		},
		common.Thumb{
			Name:   "360p",
			ID:     4,
			Mode:   common.ModeFit,
			Width:  640,
			Height: 360,
		},
		common.Thumb{
			Name:   "240p",
			ID:     5,
			Mode:   common.ModeFit,
			Width:  427,
			Height: 240,
		},
	},
}

func LoadServerConf(f string) (ServerConf, error) {
	data, err := ioutil.ReadFile(f)
	if err != nil {
		return DefaultServerConf, err
	}
	var c ServerConf
	if err := yaml.Unmarshal(data, &c); err != nil {
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
