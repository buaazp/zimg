package server

import (
	"fmt"
	"net/http"
	"strconv"
)

type ConvertModeType uint32

const (
	ModeFill ConvertModeType = iota
	ModeFit
	ModeStretch
	ModeThumb
	MaxUint32 = uint32((1 << 32) - 1)
)

var ConvertModeTypeName = map[uint32]string{
	0: "ModeFill",
	1: "ModeFit",
	2: "ModeStretch",
	3: "ModeThumb",
}

func (t ConvertModeType) String() string {
	str, ok := ConvertModeTypeName[uint32(t)]
	if !ok {
		return strconv.Itoa(int(t))
	}
	return str
}

type GravityType uint32

const (
	Face GravityType = iota
	Center
	North
	South
	West
	East
	NorthWest
	NorthEast
	SouthWest
	SouthEast
)

var GravityTypeName = map[uint32]string{
	0: "Face",
	1: "Center",
	2: "North",
	3: "South",
	4: "West",
	5: "East",
	6: "NorthWest",
	7: "NorthEast",
	8: "SouthWest",
	9: "SouthEast",
}

func (t GravityType) String() string {
	str, ok := GravityTypeName[uint32(t)]
	if !ok {
		return strconv.Itoa(int(t))
	}
	return str
}

type ConvertParam struct {
	Key       string          `json:"key,omitempty"`
	Mode      ConvertModeType `json:"mode"`
	Width     uint32          `json:"width"`
	Height    uint32          `json:"height"`
	Gravity   GravityType     `json:"gravity,omitempty"`
	Angle     uint32          `json:"angle,omitempty"`
	HasPoint1 bool            `json:"has_point1,omitempty"`
	X1        uint32          `json:"x1,omitempty"`
	Y1        uint32          `json:"y1,omitempty"`
	HasPoint2 bool            `json:"has_point2,omitempty"`
	X2        uint32          `json:"x2,omitempty"`
	Y2        uint32          `json:"y2,omitempty"`
}

func GetConvertParam(r *http.Request) (*ConvertParam, error) {
	var cp ConvertParam
	paramNames := []string{"m", "w", "h", "g", "a", "x1", "y1", "x2", "y2"}
	hasParam := false
	for _, name := range paramNames {
		pstr := r.FormValue(name)
		if pstr != "" {
			pint, err := strconv.ParseInt(pstr, 10, 0)
			if err != nil {
				return nil, err
			}
			if pint < 0 || pint >= int64(MaxUint32) {
				return nil, fmt.Errorf("convert param %s illegal %d", name, pint)
			}
			value := uint32(pint)
			hasParam = true

			switch name {
			case "m":
				if int(value) >= len(ConvertModeTypeName) {
					return nil, fmt.Errorf("illegal convert mode: %v", value)
				}
				mt := ConvertModeType(value)
				cp.Mode = mt
			case "w":
				cp.Width = value
			case "h":
				cp.Height = value
			case "g":
				if int(value) >= len(GravityTypeName) {
					return nil, fmt.Errorf("illegal gravity type: %v", value)
				}
				gt := GravityType(value)
				cp.Gravity = gt
			case "a":
				if value > 360 {
					return nil, fmt.Errorf("convert param %s illegal %d", name, pint)
				}
				cp.Angle = value
			case "x1":
				cp.HasPoint1 = true
				cp.X1 = value
			case "y1":
				cp.HasPoint1 = true
				cp.Y1 = value
			case "x2":
				cp.HasPoint2 = true
				cp.X2 = value
			case "y2":
				cp.HasPoint2 = true
				cp.Y2 = value
			default:
				return nil, fmt.Errorf("uncached param name: %s", name)
			}
		}
	}
	if cp.HasPoint2 {
		if !cp.HasPoint1 {
			return nil, fmt.Errorf("point1 should provide when point2 is set")
		}
		if cp.X1 >= cp.X2 || cp.Y1 >= cp.Y2 {
			return nil, fmt.Errorf("point2 [%d,%d] should bigger than point1 [%d,%d]", cp.X2, cp.Y2, cp.X1, cp.Y1)
		}
	}

	if !hasParam {
		return nil, nil
	}
	return &cp, nil
}

type Property map[string]string

type ImageResult struct {
	Key        string   `json:"key"`                  // key of the image
	Previews   []string `json:"previews,omitempty"`   // previews of the image
	MTime      int64    `json:"mtime"`                // UnixNano of the image
	Width      uint     `json:"width"`                // width of the image
	Height     uint     `json:"height"`               // height of the image
	Format     string   `json:"format"`               // format of the image
	Properties Property `json:"properties,omitempty"` // Properties of the Image
}
