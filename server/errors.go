package server

import "errors"

var (
	ErrNotDone        = errors.New("this function is not come true")
	ErrNoKey          = errors.New("no key in url")
	ErrNotAllowed     = errors.New("file type is not allowed")
	ErrObjectTooLarge = errors.New("object too large")
	ErrObjectNotFound = errors.New("object no found")
	ErrObjectDeleted  = errors.New("object deleted")
)
