package server

type Thumb struct {
	Name    string          `yaml:"name"`
	ID      int             `yaml:"id"`
	Mode    ConvertModeType `yaml:"mode"`
	Gravity GravityType     `yaml:"gravity,omitempty"`
	Width   uint            `yaml:"width"`
	Height  uint            `yaml:"height"`
	Pixels  uint            `yaml:"pixels,omitempty"`
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
