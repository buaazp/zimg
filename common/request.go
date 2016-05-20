package common

import (
	"fmt"
	"net/http"
	"strconv"

	"github.com/buaazp/zimg/util"
)

type ConvertModeType uint

const (
	ModeFill ConvertModeType = iota
	ModeFit
	ModeStretch
)

var ConvertModeTypeName = map[ConvertModeType]string{
	ModeFill:    "ModeFill",
	ModeFit:     "ModeFit",
	ModeStretch: "ModeStretch",
}

func (t ConvertModeType) String() string {
	str, ok := ConvertModeTypeName[t]
	if !ok {
		return strconv.Itoa(int(t))
	}
	return str
}

type GravityType uint

const (
	Center GravityType = iota
	North
	South
	West
	East
	NorthWest
	NorthEast
	SouthWest
	SouthEast
)

var GravityTypeName = map[GravityType]string{
	Center:    "Center",
	North:     "North",
	South:     "South",
	West:      "West",
	East:      "East",
	NorthWest: "NorthWest",
	NorthEast: "NorthEast",
	SouthWest: "SouthWest",
	SouthEast: "SouthEast",
}

func (t GravityType) String() string {
	str, ok := GravityTypeName[t]
	if !ok {
		return strconv.Itoa(int(t))
	}
	return str
}

type ConvertParam struct {
	Key        string          `json:"key,omitempty"`
	Mode       ConvertModeType `json:"mode"`
	Width      uint            `json:"width"`
	Height     uint            `json:"height"`
	LongSide   uint            `json:"long_side,omitempty"`
	ShortSide  uint            `json:"short_side,omitempty"`
	Gravity    GravityType     `json:"gravity,omitempty"`
	Angle      float64         `json:"angle,omitempty"`
	RelativeX  float64         `json:"relative_x,omitempty"`
	RelativeY  float64         `json:"relative_y,omitempty"`
	RelativeW  float64         `json:"relative_w,omitempty"`
	RelativeH  float64         `json:"relative_h,omitempty"`
	Format     string          `json:"format"`
	FormatOnly bool            `json:"format_only"`
}

var (
	paramNames = []string{"m", "w", "h", "ls", "ss", "g", "a", "rx", "ry", "rw", "rh"}
)

func GetConvertParam(r *http.Request, key, ext string) (*ConvertParam, error) {
	var cp ConvertParam
	cp.Key = key

	hasFormat := false
	if ext != "" {
		format := util.GetImageType(ext)
		if format == "" {
			return nil, fmt.Errorf("convert param image type [%s] not supported", ext)
		}
		cp.Format = format
		hasFormat = true
	}

	hasParam := false
	for _, name := range paramNames {
		pstr := r.FormValue(name)
		if pstr != "" {
			pf, err := strconv.ParseFloat(pstr, 0)
			if err != nil {
				return nil, err
			}
			if pf < 0 {
				return nil, fmt.Errorf("convert param %s illegal %f", name, pf)
			}
			hasParam = true

			switch name {
			case "m":
				if int(pf) >= len(ConvertModeTypeName) {
					return nil, fmt.Errorf("illegal convert mode: %v", pf)
				}
				mt := ConvertModeType(pf)
				cp.Mode = mt
			case "w":
				cp.Width = uint(pf)
			case "h":
				cp.Height = uint(pf)
			case "ls":
				cp.LongSide = uint(pf)
			case "ss":
				cp.ShortSide = uint(pf)
			case "g":
				if int(pf) >= len(GravityTypeName) {
					return nil, fmt.Errorf("illegal gravity type: %v", pf)
				}
				gt := GravityType(pf)
				cp.Gravity = gt
			case "a":
				if pf > 360 {
					return nil, fmt.Errorf("convert param %s illegal %f", name, pf)
				}
				cp.Angle = pf
			case "rx":
				if pf > 1.1 {
					return nil, fmt.Errorf("convert param %s illegal %f", name, pf)
				} else if pf > 1.0 {
					cp.RelativeX = 1.0
				} else {
					cp.RelativeX = pf
				}
			case "ry":
				if pf > 1.1 {
					return nil, fmt.Errorf("convert param %s illegal %f", name, pf)
				} else if pf > 1.0 {
					cp.RelativeY = 1.0
				} else {
					cp.RelativeY = pf
				}
			case "rw":
				if pf > 1.1 {
					return nil, fmt.Errorf("convert param %s illegal %f", name, pf)
				} else if pf > 1.0 {
					cp.RelativeW = 1.0
				} else {
					cp.RelativeW = pf
				}
			case "rh":
				if pf > 1.1 {
					return nil, fmt.Errorf("convert param %s illegal %f", name, pf)
				} else if pf > 1.0 {
					cp.RelativeH = 1.0
				} else {
					cp.RelativeH = pf
				}
			default:
				return nil, fmt.Errorf("uncached param name: %s", name)
			}
		}
	}

	if cp.NeedCrop() && cp.LackCrop() {
		return nil, fmt.Errorf("convert param rw/rh should > 0")
	}
	if !hasFormat && !hasParam {
		return nil, nil
	}
	if !hasParam && hasFormat {
		cp.FormatOnly = true
	}
	if (cp.LongSide != 0 || cp.ShortSide != 0) && cp.Mode != ModeFit {
		if cp.Width == 0 && cp.Height == 0 {
			cp.Mode = ModeFit
		}
	}

	return &cp, nil
}

func (cp *ConvertParam) NeedCrop() bool {
	return cp.RelativeW+cp.RelativeH > 0
}

func (cp *ConvertParam) LackCrop() bool {
	return cp.RelativeW*cp.RelativeH == 0
}

func (cp *ConvertParam) OutOfSize(limit uint) bool {
	return cp.Width > limit || cp.Height > limit
}

type Property map[string]string

// ObjectResult the repsonse of object PUT / POST / DELETE api
type ObjectResult struct {
	Key   string `json:"key"`   // result of ObjectParam.Encode()
	MTime int64  `json:"mtime"` // UnixNano of the Object
}

type ImageResult struct {
	Key        string            `json:"key"`                  // key of the image
	Format     string            `json:"format"`               // format of the image
	MTime      int64             `json:"mtime,omitempty"`      // unixnano timestamp of the image
	Width      uint              `json:"width,omitempty"`      // width of the image
	Height     uint              `json:"height,omitempty"`     // height of the image
	Thumbs     map[string]string `json:"thumbs,omitempty"`     // thumb fids of the image
	Properties Property          `json:"properties,omitempty"` // properties of the Image
}
