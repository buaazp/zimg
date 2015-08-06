package main

import (
	"flag"
	"fmt"
	"io"
	"log"
	"os"
	"os/signal"
	"runtime"
	"strings"
	"syscall"
	"text/template"
	"unicode"
	"unicode/utf8"
)

// Command defines a usage of zimg and its func
type Command struct {
	// Run runs the command.
	// The args are the arguments after the command name.
	Run func(cmd *Command, args []string)

	// UsageLine is the one-line usage message.
	// The first word in the line is taken to be the command name.
	UsageLine string

	// Short description
	Short string

	// Flag is a set of flags specific to this command.
	Flag flag.FlagSet
}

// Name returns the command's name: the first word in the usage line.
func (c *Command) Name() string {
	name := c.UsageLine
	i := strings.Index(name, " ")
	if i >= 0 {
		name = name[:i]
	}
	return name
}

// Usage displays the usage of a command and exit
func (c *Command) Usage() {
	fmt.Fprintf(os.Stderr, "usage: zimg %s\n", c.UsageLine)
	fmt.Fprintf(os.Stderr, "desc : %s\n", c.Short)
	c.Flag.PrintDefaults()
	os.Exit(2)
}

// Runnable reports whether the command can be run; otherwise
// it is a documentation pseudo-command such as importpath.
func (c *Command) Runnable() bool {
	return c.Run != nil
}

var commands = []*Command{
	cmdVersion,
	cmdServer,
	cmdConfig,
}

// OnExitSignal catchs the signals of syscall and deal with them
func OnExitSignal(fn func()) {
	c := make(chan os.Signal, 1)
	signal.Notify(c,
		syscall.SIGHUP,
		syscall.SIGINT,
		syscall.SIGTERM,
		syscall.SIGQUIT)
	go func() {
		log.Println("recv signal:", <-c)
		fn()
		os.Exit(0)
	}()
}

func main() {
	runtime.GOMAXPROCS(runtime.NumCPU())
	log.SetPrefix("[zimg] ")
	log.SetFlags(log.LstdFlags | log.Lshortfile | log.Lmicroseconds)

	flag.Usage = usage
	flag.Parse()

	args := flag.Args()
	if len(args) < 1 {
		usage()
	}

	if args[0] == "help" {
		help(args[1:])
		return
	}

	for _, cmd := range commands {
		if cmd.Name() == args[0] && cmd.Run != nil {
			cmd.Flag.Usage = func() { cmd.Usage() }
			cmd.Flag.Parse(args[1:])
			args = cmd.Flag.Args()
			cmd.Run(cmd, args)
			os.Exit(0)
		}
	}

	fmt.Fprintf(os.Stderr, "zimg: unknown command %q\nRun 'zimg help' for usage.\n", args[0])
	os.Exit(1)
}

var usageTemplate = `
zimg: image store and process server

Usage:

    zimg command [arguments]

The commands are:
{{range .}}{{if .Runnable}}
    {{.Name | printf "%-11s"}} {{.Short}}{{end}}{{end}}

Use "zimg help [command]" for more information about a command.

`

// tmpl executes the given template text on data, writing the result to w.
func tmpl(w io.Writer, text string, data interface{}) {
	t := template.New("top")
	t.Funcs(template.FuncMap{"trim": strings.TrimSpace, "capitalize": capitalize})
	template.Must(t.Parse(text))
	if err := t.Execute(w, data); err != nil {
		panic(err)
	}
}

func capitalize(s string) string {
	if s == "" {
		return s
	}
	r, n := utf8.DecodeRuneInString(s)
	return string(unicode.ToTitle(r)) + s[n:]
}

func usage() {
	tmpl(os.Stderr, usageTemplate, commands)
	os.Exit(2)
}

func help(args []string) {
	if len(args) == 0 {
		usage()
		return
	}
	if len(args) != 1 {
		fmt.Fprintf(os.Stderr, "usage: zimg help command\n\nToo many arguments given.\n")
		os.Exit(2)
	}

	arg := args[0]

	for _, cmd := range commands {
		if cmd.Name() == arg {
			cmd.Usage()
			return
		}
	}

	fmt.Fprintf(os.Stderr, "Unknown cmd %#q\n", arg)
	os.Exit(2)
}
