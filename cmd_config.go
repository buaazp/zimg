package main

import (
	"fmt"
	"os"
	"path"

	"github.com/buaazp/zimg/conf"
	"gopkg.in/yaml.v2"
)

var cmdConfig = &Command{
	Run:       runConfig,
	UsageLine: "config [dir]",
	Short:     "generate default zimg configurations",
}

type defaultConfig struct {
	fileName string
	config   interface{}
}

func exportDefaultConf(c defaultConfig) error {
	f, err := os.Create(c.fileName)
	if err != nil {
		return err
	}
	defer f.Close()

	data, err := yaml.Marshal(c.config)
	if err != nil {
		return err
	}

	_, err = f.Write(data)
	return err
}

func runConfig(cmd *Command, args []string) {
	var filePath string
	if len(args) > 0 {
		filePath = args[0]
	}
	defaultConfigs := []defaultConfig{
		defaultConfig{
			fileName: path.Join(filePath, "zimg.yaml"),
			config:   conf.DefaultServerConf,
		},
	}

	for _, defConf := range defaultConfigs {
		err := exportDefaultConf(defConf)
		if err != nil {
			fmt.Fprintf(os.Stderr, "zimg config %s failed: %s\n", defConf.fileName, err)
			os.Exit(-1)
		}
		fmt.Printf("zimg config succ: %s\n", defConf.fileName)
	}
	return
}
