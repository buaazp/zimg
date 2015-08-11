package util

import "fmt"

var (
	kb float64 = 1024
	mb         = 1024 * kb
	gb         = 1024 * mb
	tb         = 1024 * gb
	pb         = 1024 * tb
)

func ToHuman(size int64) string {
	fsize := float64(size)

	var toHuman string
	switch {
	case fsize > pb:
		toHuman = fmt.Sprintf("%.3fPB", fsize/pb)
	case fsize > tb:
		toHuman = fmt.Sprintf("%.3fTB", fsize/tb)
	case fsize > gb:
		toHuman = fmt.Sprintf("%.3fGB", fsize/gb)
	case fsize > mb:
		toHuman = fmt.Sprintf("%.3fMB", fsize/mb)
	case fsize > kb:
		toHuman = fmt.Sprintf("%.3fKB", fsize/kb)
	default:
		toHuman = fmt.Sprintf("%dB", size)
	}
	return toHuman
}
