package server

import (
	"fmt"
	"log"
	"math"

	"m2cv/service/face"

	"github.com/gographics/imagick/imagick"
)

const (
	NormalRatio float64 = 3.0
)

var (
	MaxConfThumb Thumb
)

func init() {
	imagick.Initialize()
	// defer imagick.Terminate()
}

func ImagickInfo(blob []byte) (rst ImageResult, err error) {
	// log.Printf("%s imagick info...", key)
	im := imagick.NewMagickWand()
	defer im.Destroy()

	err = im.ReadImageBlob(blob)
	if err != nil {
		return
	}

	ppt := imGetExif(im)
	rst.Width = im.GetImageWidth()
	rst.Height = im.GetImageHeight()
	rst.Format = im.GetImageFormat()
	rst.Properties = ppt
	return
}

func imGetExif(im *imagick.MagickWand) Property {
	ppt := make(Property)
	pNames := im.GetImageProperties("*")
	for _, name := range pNames {
		pValue := im.GetImageProperty(name)
		// log.Printf("properties[%d]: %s: %s", i, name, pValue)
		ppt[name] = pValue
	}
	return ppt
}

func ImConvert(blob []byte, thumbs []Thumb, key string) (map[string][]byte, error) {
	// start := time.Now()
	// log.Printf("convert start: %v", time.Since(start))
	im := imagick.NewMagickWand()
	// log.Printf("convert new wand: %v", time.Since(start))
	defer im.Destroy()

	err := im.ReadImageBlob(blob)
	if err != nil {
		return nil, err
	}
	// log.Printf("convert read blob: %v", time.Since(start))

	imCols := im.GetImageWidth()
	imRows := im.GetImageHeight()
	// log.Printf("cols=%d, rows=%d", imCols, imRows)
	var imRatio float64
	if imCols > imRows {
		imRatio = float64(imCols) / float64(imRows)
	} else {
		imRatio = float64(imRows) / float64(imCols)
	}

	imPixels := imCols * imRows
	if imRatio < NormalRatio {
		if imCols > MaxConfThumb.Width || imRows > MaxConfThumb.Height {
			err = ImFitSideResize(im, MaxConfThumb)
		}
	} else {
		if imPixels > MaxConfThumb.Pixels {
			err = ImFitPixelResize(im, MaxConfThumb)
		}
	}
	if err != nil {
		return nil, err
	}

	// image auto-orient
	err = autoRotate(im)
	if err != nil {
		return nil, err
	}
	// log.Printf("convert rotate: %v", time.Since(start))

	// convert CMYK's icc to sRGB
	// profiles := im.GetImageProfiles("*")
	// log.Printf("profiles: %+v", profiles)
	iccProfile := im.GetImageProfile("icc")
	// log.Printf("icc: %+v", len(iccProfile))
	if iccProfile != "" && iccProfile != srgbIcc {
		// if colorspace is CMYK and has ICC,
		// the colorspace will convert to sRGB
		// if has not ICC, colorspace will still be CMYK
		// log.Printf("%s has a private icc profile: %d", key, len(iccProfile))
		err = im.ProfileImage("icc", _srgbIcc)
		if err != nil {
			return nil, err
		}
	}
	// log.Printf("convert profile: %v", time.Since(start))

	// strip all infomation of image
	err = im.StripImage()
	if err != nil {
		return nil, err
	}
	// log.Printf("convert strip: %v", time.Since(start))

	format := im.GetImageFormat()
	// log.Printf("format: %s", format)
	switch format {
	case "GIF":
		// Composites a set of images while respecting any page
		// offsets and disposal methods
		im = im.CoalesceImages()
		// log.Printf("convert coalesce: %v", time.Since(start))
		// Maybe not needed:
		// select the smallest cropped image to replace each frame
		// im = im.OptimizeImageLayers()
		// log.Printf("convert optimize layer: %v", time.Since(start))
		// w, h, x, y, err := im.GetImagePage()
		// if err != nil {
		// 	return nil, err
		// }
		// log.Println(w, h, x, y)
		// err = im.ResetImagePage("0x0+0+15!")
	case "PNG":
		// convert transparent to white background
		background := imagick.NewPixelWand()
		defer background.Destroy()
		succ := background.SetColor("white")
		if !succ {
			return nil, fmt.Errorf("PNG set color white failed")
		}
		err = im.SetImageBackgroundColor(background)
		if err != nil {
			return nil, err
		}
		im = im.MergeImageLayers(imagick.IMAGE_LAYER_FLATTEN)
		// log.Printf("convert set white background: %v", time.Since(start))
	default:
	}

	// colorspcae := im.GetImageColorspace()
	// log.Printf("colorspace: %+v", colorspcae)
	// log.Printf("convert colorspace to: %+v", imagick.COLORSPACE_RGB)
	// err = im.SetImageColorspace(imagick.COLORSPACE_RGB)
	// if err != nil {
	// 	return nil, err
	// }
	// log.Printf("convert colorspace: %v", time.Since(start))

	err = im.SetImageFormat(DefaultThumbType)
	if err != nil {
		return nil, err
	}
	// log.Printf("convert format: %v", time.Since(start))
	// format2 := im.GetImageFormat()
	// log.Printf("format2: %s", format2)

	// convert image to progressive jpeg
	// progressive reduce jpeg size incidentally
	// but convert elapsed time increase about 10%
	err = im.SetInterlaceScheme(imagick.INTERLACE_PLANE)
	if err != nil {
		return nil, err
	}
	// log.Printf("convert interlace: %v", time.Since(start))

	mBody := im.GetImageBlob()
	// log.Printf("%s %s mBody: %d", key, MaxConfThumb.Name, len(mBody))
	// log.Printf("convert get %s blob: %v", MaxConfThumb.Name, time.Since(start))

	imCols = im.GetImageWidth()
	imRows = im.GetImageHeight()
	// log.Printf("cols=%d, rows=%d", imCols, imRows)
	tBody := make(map[string][]byte)
L1:
	for _, tb := range thumbs {
		if tb.Width == 0 && tb.Height == 0 {
			continue L1
		}
		nKey := fmt.Sprintf("%s,%d", key, tb.ID)

		if tb.Name == MaxConfThumb.Name && tb.ID == MaxConfThumb.ID {
			tBody[nKey] = mBody
			log.Printf("%s %s sBody: %d", nKey, tb.Name, len(mBody))
			continue L1
		}

		if (tb.Width == 0 && tb.Height < imRows) ||
			(tb.Height == 0 && tb.Width < imCols) ||
			((tb.Width > 0 && tb.Height > 0) && (tb.Width < imCols || tb.Height < imRows)) {
			nBody, err := ImReduce(mBody, tb, nKey)
			if err != nil {
				return nil, err
			}
			// log.Printf("convert reduce %s: %v", tb.Name, time.Since(start))
			tBody[nKey] = nBody
			log.Printf("%s %s nBody: %d", nKey, tb.Name, len(nBody))
		} else {
			tBody[nKey] = mBody
			log.Printf("%s %s sBody: %d", nKey, tb.Name, len(mBody))
		}
	}

	return tBody, nil
}

func autoRotate(im *imagick.MagickWand) (err error) {
	ot := im.GetImageOrientation()
	switch ot {
	case imagick.ORIENTATION_UNDEFINED: // 未设置
		// log.Printf("0:ORIENTATION_UNDEFINED")
	case imagick.ORIENTATION_TOP_LEFT: // 正常
		// log.Printf("1:ORIENTATION_TOP_LEFT")
	case imagick.ORIENTATION_TOP_RIGHT: // 水平镜像
		// log.Printf("2:ORIENTATION_TOP_RIGHT")
		err = im.FlopImage()
	case imagick.ORIENTATION_BOTTOM_RIGHT: // 顺时针旋转180度
		// log.Printf("3:ORIENTATION_BOTTOM_RIGHT")
		background := imagick.NewPixelWand()
		defer background.Destroy()
		err = im.RotateImage(background, 180)
	case imagick.ORIENTATION_BOTTOM_LEFT: // 垂直镜像
		// log.Printf("4:ORIENTATION_BOTTOM_LEFT")
		err = im.FlipImage()
	case imagick.ORIENTATION_LEFT_TOP: // 顺时针旋转90度，水平镜像
		// log.Printf("5:ORIENTATION_LEFT_TOP")
		background := imagick.NewPixelWand()
		defer background.Destroy()
		err = im.RotateImage(background, 90)
		if err != nil {
			return
		}
		err = im.FlopImage()
	case imagick.ORIENTATION_RIGHT_TOP: // 顺时针旋转90度
		// log.Printf("6:ORIENTATION_RIGHT_TOP")
		background := imagick.NewPixelWand()
		defer background.Destroy()
		err = im.RotateImage(background, 90)
	case imagick.ORIENTATION_RIGHT_BOTTOM: // 顺时针旋转90度，垂直镜像
		// log.Printf("7:ORIENTATION_RIGHT_BOTTOM")
		background := imagick.NewPixelWand()
		defer background.Destroy()
		err = im.RotateImage(background, 90)
		if err != nil {
			return
		}
		err = im.FlipImage()
	case imagick.ORIENTATION_LEFT_BOTTOM: // 顺时针旋转270度
		// log.Printf("8:ORIENTATION_LEFT_BOTTOM")
		background := imagick.NewPixelWand()
		defer background.Destroy()
		err = im.RotateImage(background, 270)
	default: // 读取 EXIF Orientation 错误
		err = fmt.Errorf("read exif orientation error")
	}
	return
}

func ImReduce(blob []byte, tb Thumb, key string) ([]byte, error) {
	// start := time.Now()
	// log.Printf("reduce start: %v", time.Since(start))
	im := imagick.NewMagickWand()
	defer im.Destroy()
	// log.Printf("reduce new wand: %v", time.Since(start))

	err := im.ReadImageBlob(blob)
	if err != nil {
		return nil, err
	}
	// log.Printf("reduce read: %v", time.Since(start))

	// defer func() {
	// 	log.Printf("reduce finish: %v", time.Since(start))
	// }()

	if tb.Width == 0 || tb.Height == 0 {
		return ImJustResize(im, tb)
	}

	switch tb.Mode {
	case ModeFill:
		return ImFillReduce(im, tb, key)
	case ModeFit:
		return ImFitReduce(im, tb)
	default:
		return nil, fmt.Errorf("unknown convert mode: %+v", tb.Mode)
	}

	return nil, fmt.Errorf("uncanny error: it shouldn't happen!")
}

func ImJustResize(im *imagick.MagickWand, tb Thumb) ([]byte, error) {
	// log.Printf("just scaling...")
	imCols := im.GetImageWidth()
	imRows := im.GetImageHeight()

	imRatio := float64(imCols) / float64(imRows)
	if tb.Width == 0 {
		tb.Width = uint(math.Floor(float64(tb.Height) * imRatio))
	}
	if tb.Height == 0 {
		tb.Height = uint(math.Floor(float64(tb.Width) / imRatio))
	}

	err := im.ResizeImage(tb.Width, tb.Height, imagick.FILTER_LANCZOS, 0.8)
	if err != nil {
		return nil, err
	}
	return im.GetImageBlob(), nil
}

func outOfImage(cols, rows, x, y uint) bool {
	return x > cols || y > rows
}

func ImFillReduce(im *imagick.MagickWand, tb Thumb, key string) ([]byte, error) {
	if tb.Gravity == Face {
		return imFillFace(im, tb, key)
	}

	return imFillResize(im, tb)
}

func imFillFace(im *imagick.MagickWand, tb Thumb, key string) ([]byte, error) {
	// step 1: resize image to the size contains target
	imCols := im.GetImageWidth()
	imRows := im.GetImageHeight()
	// log.Println("old size: ", imCols, imRows)

	var cols, rows uint
	xf := float64(tb.Width) / float64(imCols)
	yf := float64(tb.Height) / float64(imRows)
	if xf > yf {
		cols = tb.Width
		rows = uint(math.Floor(float64(imRows) * xf))
	} else if yf > xf {
		cols = uint(math.Floor(float64(imCols) * yf))
		rows = tb.Height
	} else {
		cols = tb.Width
		rows = tb.Height
	}

	err := im.ResizeImage(cols, rows, imagick.FILTER_LANCZOS, 0.8)
	if err != nil {
		return nil, err
	}
	// log.Println("new size: ", cols, rows)
	data := im.GetImageBlob()
	// step 2: send to cv service to get the postion of face
	resp, err := face.FaceDetectThrift(data, key)
	if err != nil {
		return nil, err
	}
	// step 3: calculate the target range and crop it
	var x, y uint
	faceNum := resp.GetFaceNum()
	if faceNum >= 1 && faceNum <= 3 {
		x1 := uint(resp.GetLeft())
		y1 := uint(resp.GetTop())
		x2 := uint(resp.GetRight())
		y2 := uint(resp.GetBottom())
		// log.Println("face ", x1, y1, x2, y2)
		faceCols := x2 - x1
		faceRows := y2 - y1
		if cols > tb.Width {
			croped := cols - tb.Width
			if cols > faceCols {
				padding := cols - faceCols
				x = uint(math.Floor(float64(croped) * float64(x1) / float64(padding)))
			} else {
				x = 0
			}
			y = 0
		} else if rows > tb.Height {
			croped := rows - tb.Height
			if rows > faceRows {
				padding := rows - faceRows
				y = uint(math.Floor(float64(croped) * float64(y1) / float64(padding)))
			} else {
				y = 0
			}
			x = 0
		}
	} else {
		x = (cols - tb.Width) / 2
		y = (rows - tb.Height) / 2
	}

	// log.Println("crop ", tb.X1, tb.Y1, tb.Width, tb.Height)
	return imJustCrop(im, tb, x, y)
}

func imJustCrop(im *imagick.MagickWand, tb Thumb, x, y uint) ([]byte, error) {
	// log.Printf("just croping...")
	imCols := im.GetImageWidth()
	imRows := im.GetImageHeight()
	if outOfImage(imCols, imRows, x, y) {
		return nil, fmt.Errorf("point1 [%d,%d] is out of image(%d, %d)", x, y, imCols, imRows)
	}

	if outOfImage(imCols, imRows, x+tb.Width, y+tb.Height) {
		return nil, fmt.Errorf("point2 [%d,%d] is out of image(%d, %d)", x+tb.Width, y+tb.Height, imCols, imRows)
	}

	err := im.CropImage(tb.Width, tb.Height, int(x), int(y))
	if err != nil {
		return nil, err
	}
	return im.GetImageBlob(), nil
}

func imFillResize(im *imagick.MagickWand, tb Thumb) ([]byte, error) {
	// log.Printf("fill resizing...")
	err := imFillCrop(im, tb)
	if err != nil {
		return nil, err
	}

	err = im.ResizeImage(tb.Width, tb.Height, imagick.FILTER_LANCZOS, 0.8)
	if err != nil {
		return nil, err
	}

	return im.GetImageBlob(), nil
}

func imFillCrop(im *imagick.MagickWand, tb Thumb) error {
	// log.Printf("fill croping...")
	imCols := im.GetImageWidth()
	imRows := im.GetImageHeight()

	xf := float64(tb.Width) / float64(imCols)
	yf := float64(tb.Height) / float64(imRows)

	var cols, rows uint
	if xf > yf {
		cols = imCols
		rows = uint(math.Floor(float64(tb.Height) / xf))
	} else if yf > xf {
		cols = uint(math.Floor(float64(tb.Width) / yf))
		rows = imRows
	} else {
		return nil
	}

	var x, y uint
	switch tb.Gravity {
	case Center:
		x += (imCols - cols) / 2
		y += (imRows - rows) / 2
	case North:
		x += (imCols - cols) / 2
		y += 0
	case South:
		x += (imCols - cols) / 2
		y += imRows - rows
	case West:
		x += 0
		y += (imRows - rows) / 2
	case East:
		x += imCols - cols
		y += (imRows - rows) / 2
	case NorthWest:
		x += 0
		y += 0
	case NorthEast:
		x += imCols - cols
		y += 0
	case SouthWest:
		x += 0
		y += imRows - rows
	case SouthEast:
		x += imCols - cols
		y += imRows - rows
	default:
		return fmt.Errorf("illegal gravity mode: %v", tb.Gravity)
	}

	// log.Printf("crop: %d %d %d %d", x, y, cols, rows)
	err := im.CropImage(cols, rows, int(x), int(y))
	if err != nil {
		return err
	}

	return nil
}

func ImFitReduce(im *imagick.MagickWand, tb Thumb) ([]byte, error) {
	imCols := im.GetImageWidth()
	imRows := im.GetImageHeight()
	// log.Printf("cols=%d, rows=%d", imCols, imRows)
	var imRatio float64
	if imCols > imRows {
		imRatio = float64(imCols) / float64(imRows)
	} else {
		imRatio = float64(imRows) / float64(imCols)
	}
	if imRatio < NormalRatio {
		return ImFitSideReduce(im, tb)
	}

	imPixels := imCols * imRows
	if imPixels > tb.Pixels {
		return ImFitPixelReduce(im, tb)
	}
	return im.GetImageBlob(), nil
}

func ImFitSideReduce(im *imagick.MagickWand, tb Thumb) ([]byte, error) {
	// log.Printf("fit side resizing...")
	err := ImFitSideResize(im, tb)
	if err != nil {
		return nil, err
	}
	// start := time.Now()
	// defer func() {
	// 	log.Printf("side reduce get blob: %v", time.Since(start))
	// }()
	return im.GetImageBlob(), nil
}

func ImFitSideResize(im *imagick.MagickWand, tb Thumb) error {
	// log.Printf("fit pixel resizing...")
	// start := time.Now()
	imCols := im.GetImageWidth()
	imRows := im.GetImageHeight()

	xf := float64(tb.Width) / float64(imCols)
	yf := float64(tb.Height) / float64(imRows)

	var cols, rows uint
	if xf < yf {
		cols = tb.Width
		rows = uint(math.Floor(float64(imRows) * xf))
	} else if yf < xf {
		cols = uint(math.Floor(float64(imCols) * yf))
		rows = tb.Height
	} else {
		cols = tb.Width
		rows = tb.Height
	}

	err := im.ResizeImage(cols, rows, imagick.FILTER_LANCZOS, 0.8)
	if err != nil {
		return err
	}
	// log.Printf("side resize resize image: %v", time.Since(start))
	return nil
}

func ImFitPixelReduce(im *imagick.MagickWand, tb Thumb) ([]byte, error) {
	// log.Printf("fit pixel resizing...")
	err := ImFitPixelResize(im, tb)
	if err != nil {
		return nil, err
	}
	// start := time.Now()
	// defer func() {
	// 	log.Printf("pixel reduce get blob: %v", time.Since(start))
	// }()
	return im.GetImageBlob(), nil
}

func ImFitPixelResize(im *imagick.MagickWand, tb Thumb) error {
	// log.Printf("fit pixel resizing...")
	// start := time.Now()
	imCols := im.GetImageWidth()
	imRows := im.GetImageHeight()
	imPixel := imCols * imRows
	pf := math.Sqrt(float64(tb.Pixels) / float64(imPixel))
	cols := uint(math.Floor(float64(imCols) * pf))
	rows := uint(math.Floor(float64(imRows) * pf))
	err := im.ResizeImage(cols, rows, imagick.FILTER_LANCZOS, 0.8)
	if err != nil {
		return err
	}
	// log.Printf("pixel resize resize image: %v", time.Since(start))
	return nil
}
