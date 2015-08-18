package server

type Thumb struct {
	Name    string          `toml:"name"`
	ID      int             `toml:"id"`
	Mode    ConvertModeType `toml:"mode"`
	Gravity GravityType     `toml:"gravity,omitempty"`
	Width   uint            `toml:"width"`
	Height  uint            `toml:"height"`
	Pixels  uint            `toml:"pixels,omitempty"`
}

func (t *Thumb) String() string {
	return t.Name
}

func (t *Thumb) IsIllegal() bool {
	if t.Name == "" {
		return true
	}
	if t.Width == 0 || t.Height == 0 {
		return true
	}
	return false
}
