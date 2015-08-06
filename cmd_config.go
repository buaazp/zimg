package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"os"
	"path"

	"zimg/conf"
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

func exportDefaultConf(conf defaultConfig) error {
	f, err := os.Create(conf.fileName)
	if err != nil {
		return err
	}
	defer f.Close()

	data, err := json.Marshal(conf.config)
	if err != nil {
		return err
	}

	var output bytes.Buffer
	err = json.Indent(&output, data, "", "  ")
	if err != nil {
		return err
	}

	_, err = f.Write(output.Bytes())
	return err
}

func runConfig(cmd *Command, args []string) {
	var filePath string
	if len(args) > 0 {
		filePath = args[0]
	}
	defaultConfigs := []defaultConfig{
		defaultConfig{
			fileName: path.Join(filePath, "zimg.json"),
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
