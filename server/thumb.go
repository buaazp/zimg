package server

type Thumb struct {
	Name    string          `json:"name" yaml:"name"`
	ID      int             `json:"id" yaml:"id"`
	Mode    ConvertModeType `json:"mode" yaml:"mode"`
	Gravity GravityType     `json:"gravity,omitempty" yaml:"gravity,omitempty"`
	Width   uint            `json:"width" yaml:"width"`
	Height  uint            `json:"height" yaml:"height"`
	Pixels  uint            `json:"pixels,omitempty" yaml:"pixels,omitempty"`
	Format  string          `json:"format,omitempty" yaml:"format,omitempty"`
	Quality uint            `json:"quality,omitempty" yaml:"quality,omitempty"`
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
