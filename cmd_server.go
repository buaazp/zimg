package main

import (
	"fmt"
	"log"
	"os"

	"zimg/conf"
	"zimg/server"
	"zimg/util"
)

func init() {
	cmdServer.Run = runServer
}

var cmdServer = &Command{
	UsageLine: "server",
	Short:     "start a zimg server",
}

var (
	c = cmdServer.Flag.String("c", "", "the zimg server conf file")
)

func runServer(cmd *Command, args []string) {
	fmt.Printf("starting zimg server v%s..\n", util.VERSION)

	sc := conf.DefaultServerConf
	if *c != "" {
		var err error
		sc, err = conf.LoadServerConf(*c)
		if err != nil {
			fmt.Printf("load conf: %s\n", err)
			os.Exit(-1)
		}
	}

	if sc.LogFile != "" {
		f, err := os.OpenFile(sc.LogFile, os.O_WRONLY|os.O_APPEND|os.O_CREATE, 0644)
		if err != nil {
			fmt.Printf("%s\r\n", err.Error())
			os.Exit(-1)
		}
		defer f.Close()
		log.SetOutput(f)
	}

	s, err := server.NewServer(sc)
	if err != nil {
		fmt.Printf("%s\r\n", err.Error())
		os.Exit(-1)
	}
	OnExitSignal(func() {
		s.Close()
	})
	fmt.Printf("listening at %s\n", sc.Host)

	if err := s.Serve(); err != nil {
		log.Fatalf("zimg server serve err: %s\n", err)
	}
}
