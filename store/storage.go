package store

const (
	S_LOCAL   = "local"
	S_BEANSDB = "beansdb"
	S_SSDB    = "ssdb"
)

type Storage interface {
	Set(data []byte) (string, error)
	Get(key string) ([]byte, error)
	Del(key string) error
}
