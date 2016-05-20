package util

import (
	"io"
	"os"
	"time"
)

// CTime returns create time of the object
func CTime(mtime int64) time.Time {
	return time.Unix(mtime/1e9, mtime%1e9)
}

type ByteReadSeeker struct {
	b   []byte
	off int64
}

func NewByteReadSeeker(b []byte) *ByteReadSeeker {
	return &ByteReadSeeker{b: b, off: 0}
}

func (r *ByteReadSeeker) Read(b []byte) (n int, err error) {
	n = copy(b, r.b[r.off:])
	r.off += int64(n)
	if n < len(b) {
		err = io.EOF
	}
	return
}

func (r *ByteReadSeeker) Seek(offset int64, whence int) (int64, error) {
	var newOffset int64
	if whence == os.SEEK_SET {
		newOffset = offset
	} else if whence == os.SEEK_CUR {
		newOffset = r.off + offset
	} else if whence == os.SEEK_END {
		newOffset = int64(len(r.b)) + offset
	} else {
		return r.off, os.ErrInvalid
	}
	if newOffset < 0 || newOffset > int64(len(r.b)) {
		return r.off, os.ErrInvalid
	}
	r.off = newOffset
	return r.off, nil
}

type SniffReader struct {
	r         io.Reader
	sniff     []byte
	sniffLeft int
}

func NewSniffReader(r io.Reader, sniff int) *SniffReader {
	return &SniffReader{r: r, sniff: make([]byte, sniff), sniffLeft: sniff}
}

func (r *SniffReader) Read(b []byte) (n int, err error) {
	n, err = r.r.Read(b)
	if n > 0 && r.sniffLeft > 0 {
		r.sniffLeft -= copy(r.sniff[len(r.sniff)-r.sniffLeft:], b[:n])
	}
	return
}

func (r *SniffReader) Bytes() []byte {
	return r.sniff[:len(r.sniff)-r.sniffLeft]
}
