package store

const (
	S_LOCAL   = "local"
	S_BEANSDB = "beansdb"
	S_SSDB    = "ssdb"
)

type Storage interface {
	// TODO: return http code in interfaces
	Set(data []byte) (string, int64, error)
	Put(key string, data []byte) (int64, error)
	Get(key string) ([]byte, int64, error)
	Del(key string) error
}
