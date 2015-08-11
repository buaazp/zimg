package main

import (
	"fmt"
	"runtime"

	"github.com/buaazp/zimg/util"
)

var (
	cmdVersion = &Command{
		Run:       runVersion,
		UsageLine: "version",
		Short:     "print zimg version",
	}
	// Git SHA Value will be set during build
	GitSHA    = "Not provided (use ./build instead of go build)"
	BuildTime = "Not provided (use ./build instead of go build)"
)

func runVersion(cmd *Command, args []string) {
	fmt.Printf("zimg v%s\n", util.VERSION)
	fmt.Printf("%10s : %s\n", "Built by", runtime.Version())
	fmt.Printf("%10s : %s\n", "Built at", BuildTime)
	fmt.Printf("%10s : %s\n", "Git SHA", GitSHA)
}
