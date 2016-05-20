package server

import (
	"fmt"
	"log"
	"math"
	"sync"

	"github.com/buaazp/zimg/common"
	"gopkg.in/gographics/imagick.v2/imagick"
)

const (
	MiddleChannelFormat = "BMP"
)

var (
	MaxConfThumb common.Thumb
	SamplePixel  uint    = 5000 * 5000
	NormalRatio  float64 = 3.0
)

func MagickInit() {
	imagick.Initialize()
}

func MagickTerm() {
	imagick.Terminate()
}

func ImagickInfo(key string, data []byte) (rst common.ImageResult, err error) {
	// log.Printf("%s imagick info...", key)
	im := imagick.NewMagickWand()
	defer im.Destroy()

	err = im.PingImageBlob(data)
	if err != nil {
		return
	}

	im.ResetIterator()
	ppt := imGetExif(im)
	rst.Width = im.GetImageWidth()
	rst.Height = im.GetImageHeight()
	rst.Format = im.GetImageFormat()
	rst.Properties = ppt
	return
}

func imGetExif(im *imagick.MagickWand) common.Property {
	ppt := make(common.Property)
	pNames := im.GetImageProperties("*")
	for _, name := range pNames {
		pValue := im.GetImageProperty(name)
		// log.Printf("properties[%d]: %s: %s", i, name, pValue)
		ppt[name] = pValue
	}
	return ppt
}

func ImConvert(blob []byte, thumbs []common.Thumb, key string) ([][]byte, error) {
	// start := time.Now()
	im := imagick.NewMagickWand()
	defer im.Destroy()
	// log.Printf("%s convert new wand: %v", key, time.Since(start))

	// start = time.Now()
	err := im.ReadImageBlob(blob)
	if err != nil {
		log.Printf("%s read blob err: %s", key, err)
		return nil, ErrBadimg
	}
	// log.Printf("%s convert read blob: %v", key, time.Since(start))

	longSide := im.GetImageWidth()
	shortSide := im.GetImageHeight()
	// log.Printf("%s cols=%d, rows=%d", key, longSide, shortSide)
	if longSide < shortSide {
		longSide, shortSide = shortSide, longSide
	}
	imRatio := float64(longSide) / float64(shortSide)

	var standardThumb common.Thumb
	if len(thumbs) == 1 {
		standardThumb = thumbs[0]
	} else {
		standardThumb = MaxConfThumb
	}

	// start = time.Now()
	if imRatio < NormalRatio {
		if longSide > standardThumb.Width || shortSide > standardThumb.Height {
			err = ImFitSideResize(im, standardThumb)
		}
	} else {
		imPixels := longSide * shortSide
		if imPixels > standardThumb.Pixels {
			err = ImFitPixelResize(im, standardThumb)
		}
	}
	if err != nil {
		return nil, err
	}
	// log.Printf("%s convert %s resize: %v", key, standardThumb.Name, time.Since(start))

	// image auto-orient
	// orientation := im.GetImageOrientation()
	// log.Printf("%s orientation: %v", key, orientation)
	// start := time.Now()
	err = im.AutoOrientImage()
	if err != nil {
		log.Printf("%s auto orient image err: %s", key, err)
	}
	// log.Printf("%s convert rotate: %v", key, time.Since(start))

	// convert CMYK's icc to sRGB
	// start = time.Now()
	// profiles := im.GetImageProfiles("*")
	// log.Printf("%s profiles: %+v", key, profiles)
	iccProfile := im.GetImageProfile("icc")
	// log.Printf("%s icc: %+v", key, len(iccProfile))
	if iccProfile != "" && iccProfile != srgbIcc {
		// if colorspace is CMYK and has ICC,
		// the colorspace will convert to sRGB
		// if has not ICC, colorspace will still be CMYK
		// log.Printf("%s has a private icc profile: %d", key, len(iccProfile))
		err = im.ProfileImage("icc", _srgbIcc)
		if err != nil {
			// log.Printf("%s profile image err: %s", key, err)
		}
	}
	// log.Printf("%s convert profile: %v", key, time.Since(start))

	// strip all infomation of image
	// start = time.Now()
	err = im.StripImage()
	if err != nil {
		log.Printf("%s strip image err: %s", key, err)
	}
	// log.Printf("%s convert strip: %v", key, time.Since(start))

	// format := im.GetImageFormat()
	// // log.Printf("%s format: %s", key, format)
	// switch format {
	// // case "JPEG":
	// // TODO: jpeg convert here
	// case "GIF":
	// 	// Composites a set of images while respecting any page
	// 	// offsets and disposal methods
	// 	gifim := im.CoalesceImages()
	// 	defer gifim.Destroy()
	// 	im = gifim
	// 	// log.Printf("%s convert coalesce: %v", key, time.Since(start))
	// 	// case "PNG":
	// 	// 	// convert transparent to white background
	// 	// 	background := imagick.NewPixelWand()
	// 	// 	defer background.Destroy()
	// 	// 	succ := background.SetColor("white")
	// 	// 	if !succ {
	// 	// 		return nil, fmt.Errorf("PNG set color white failed")
	// 	// 	}
	// 	// 	err = im.SetImageBackgroundColor(background)
	// 	// 	if err != nil {
	// 	// 		return nil, err
	// 	// 	}
	// 	// 	pngim := im.MergeImageLayers(imagick.IMAGE_LAYER_FLATTEN)
	// 	// 	defer pngim.Destroy()
	// 	// 	im = pngim
	// 	// 	// log.Printf("%s convert set white background: %v", key, time.Since(start))
	// }

	// colorspcae := im.GetImageColorspace()
	// log.Printf("%s colorspace: %+v", key, colorspcae)
	// log.Printf("%s convert colorspace to: %+v", key, imagick.COLORSPACE_RGB)
	// err = im.SetImageColorspace(imagick.COLORSPACE_RGB)
	// if err != nil {
	// 	return nil, err
	// }
	// log.Printf("%s convert colorspace: %v", key, time.Since(start))

	tBody := make([][]byte, len(thumbs))
	if len(thumbs) == 1 {
		// start = time.Now()
		nBody, err := ImEncode(im, thumbs[0].Format)
		if err != nil {
			return nil, err
		}
		log.Printf("%s %s sBody: %d", key, thumbs[0].Name, len(nBody))
		// log.Printf("%s convert encode %s: %v", key, thumbs[0].Name, time.Since(start))
		tBody[0] = nBody
		return tBody, nil
	}

	longSide = im.GetImageWidth()
	shortSide = im.GetImageHeight()
	// log.Printf("%s cols=%d, rows=%d", key, longSide, shortSide)
	if longSide < shortSide {
		longSide, shortSide = shortSide, longSide
	}

	// start := time.Now()
	err = im.SetImageFormat(MiddleChannelFormat)
	if err != nil {
		log.Printf("%s set mid format %s err: %s", key, MiddleChannelFormat, err)
	}
	// log.Printf("%s convert format: %v", key, time.Since(start))
	// start = time.Now()
	bmp := im.GetImageBlob()
	log.Printf("%s %s bmp: %d", key, standardThumb.Name, len(bmp))
	// log.Printf("%s convert get bmp blob: %v", key, time.Since(start))

	var (
		tlock   sync.Mutex
		errC    = make(chan error, len(thumbs))
		saveIds = make([]int, 0, len(thumbs))
	)
L1:
	for i, tb := range thumbs {
		if tb.Width == 0 && tb.Height == 0 {
			tlock.Lock()
			tBody[i] = nil
			tlock.Unlock()
			continue L1
		}

		if (tb.Width == 0 && tb.Height < shortSide) ||
			(tb.Height == 0 && tb.Width < longSide) ||
			((tb.Width > 0 && tb.Height > 0) && (tb.Width < longSide || tb.Height < shortSide)) {
			go func(i int, tb common.Thumb) {
				// start = time.Now()
				nBody, err := ImReduce(bmp, tb, key)
				if err != nil {
					errC <- err
					return
				} else {
					log.Printf("%s %s nBody: %d", key, tb.Name, len(nBody))
					// log.Printf("%s convert reduce %s: %v", key, tb.Name, time.Since(start))
					tlock.Lock()
					tBody[i] = nBody
					tlock.Unlock()
				}
				errC <- nil
			}(i, tb)
		} else {
			saveIds = append(saveIds, i)
		}
	}

	if len(saveIds) > 0 {
		// start = time.Now()
		nBody, err := ImEncode(im, DefaultOutputFormat)
		if err != nil {
			return nil, err
		}
		// log.Printf("%s convert encode %s: %v", key, DefaultOutputFormat, time.Since(start))
		for _, saveId := range saveIds {
			tb := thumbs[saveId]
			log.Printf("%s %s sBody: %d", key, tb.Name, len(nBody))
			tlock.Lock()
			tBody[saveId] = nBody
			tlock.Unlock()
			errC <- nil
		}
	}

	for i := 0; i < len(thumbs); i++ {
		select {
		case err := <-errC:
			if err != nil {
				log.Printf("%s errC recieved: %s", key, err)
				return nil, err
			}
		}
	}

	return tBody, nil
}

func ImEncode(im *imagick.MagickWand, tbFormat string) ([]byte, error) {
	if tbFormat == "" {
		tbFormat = DefaultOutputFormat
	}

	err := im.SetImageFormat(tbFormat)
	if err != nil {
		return nil, err
	}

	quality := im.GetImageCompressionQuality()
	if quality > DefaultCompressionQuality || quality == 0 {
		err = im.SetImageCompressionQuality(DefaultCompressionQuality)
		if err != nil {
			log.Printf("set compression quality err: %v", err)
		}
		// log.Printf("set image compression quality: %v", time.Since(start))
	}

	// convert image to progressive jpeg
	// progressive reduce jpeg size incidentally
	// but convert elapsed time increase about 10%
	err = im.SetInterlaceScheme(imagick.INTERLACE_PLANE)
	if err != nil {
		log.Printf("interlace image err: %s", err)
	}
	// log.Printf("convert interlace: %v", time.Since(start))

	return im.GetImageBlob(), nil
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
		log.Printf("illegal exif orientation err: %v", ot)
	}
	// strip image will remove this property, so this code is needn't
	// err = im.SetImageOrientation(imagick.ORIENTATION_TOP_LEFT)
	return
}

func ImReduce(blob []byte, tb common.Thumb, key string) ([]byte, error) {
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
		err = ImJustResize(im, tb)
	} else {
		switch tb.Mode {
		case common.ModeFill:
			err = ImFillReduce(im, tb, key)
		case common.ModeFit:
			err = ImFitReduce(im, tb)
		default:
			return nil, fmt.Errorf("unknown convert mode: %+v", tb.Mode)
		}
	}
	if err != nil {
		return nil, err
	}

	return ImEncode(im, tb.Format)
}

func ImJustResize(im *imagick.MagickWand, tb common.Thumb) error {
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

	if imCols*imRows > SamplePixel {
		log.Printf("%dx%d > sample pixel, sampling...", imCols, imRows)
		return im.SampleImage(tb.Width, tb.Height)
	}
	return im.ResizeImage(tb.Width, tb.Height, imagick.FILTER_LANCZOS, 0.8)
}

func ImFillReduce(im *imagick.MagickWand, tb common.Thumb, key string) error {
	return imFillResize(im, tb)
}

func imJustCrop(im *imagick.MagickWand, tb common.Thumb, x, y uint) error {
	// log.Printf("just croping...")
	imCols := im.GetImageWidth()
	imRows := im.GetImageHeight()
	if outOfImage(imCols, imRows, x, y) {
		return fmt.Errorf("point1 [%d,%d] is out of image(%d, %d)", x, y, imCols, imRows)
	}

	if outOfImage(imCols, imRows, x+tb.Width, y+tb.Height) {
		return fmt.Errorf("point2 [%d,%d] is out of image(%d, %d)", x+tb.Width, y+tb.Height, imCols, imRows)
	}

	return im.CropImage(tb.Width, tb.Height, int(x), int(y))
}

func imFillResize(im *imagick.MagickWand, tb common.Thumb) error {
	// log.Printf("fill resizing...")
	err := imFillCrop(im, tb)
	if err != nil {
		return err
	}

	imCols := im.GetImageWidth()
	imRows := im.GetImageHeight()
	if imCols*imRows > SamplePixel {
		log.Printf("%dx%d > sample pixel, sampling...", imCols, imRows)
		return im.SampleImage(tb.Width, tb.Height)
	}
	return im.ResizeImage(tb.Width, tb.Height, imagick.FILTER_LANCZOS, 0.8)
}

func imFillCrop(im *imagick.MagickWand, tb common.Thumb) error {
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
	case common.Center:
		x += (imCols - cols) / 2
		y += (imRows - rows) / 2
	case common.North:
		x += (imCols - cols) / 2
		y += 0
	case common.South:
		x += (imCols - cols) / 2
		y += imRows - rows
	case common.West:
		x += 0
		y += (imRows - rows) / 2
	case common.East:
		x += imCols - cols
		y += (imRows - rows) / 2
	case common.NorthWest:
		x += 0
		y += 0
	case common.NorthEast:
		x += imCols - cols
		y += 0
	case common.SouthWest:
		x += 0
		y += imRows - rows
	case common.SouthEast:
		x += imCols - cols
		y += imRows - rows
	default:
		return fmt.Errorf("illegal gravity mode: %v", tb.Gravity)
	}

	// log.Printf("crop: %d %d %d %d", x, y, cols, rows)
	return im.CropImage(cols, rows, int(x), int(y))
}

func ImFitReduce(im *imagick.MagickWand, tb common.Thumb) error {
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
		return ImFitSideResize(im, tb)
	}

	imPixels := imCols * imRows
	if imPixels > tb.Pixels {
		return ImFitPixelResize(im, tb)
	}
	return nil
}

func ImFitSideResize(im *imagick.MagickWand, tb common.Thumb) error {
	// log.Printf("fit pixel resizing...")
	// start := time.Now()
	imCols := im.GetImageWidth()
	imRows := im.GetImageHeight()

	tbCols := tb.Width
	tbRows := tb.Height
	if imCols < imRows {
		tbCols = tb.Height
		tbRows = tb.Width
	}
	xf := float64(tbCols) / float64(imCols)
	yf := float64(tbRows) / float64(imRows)
	var cols, rows uint
	if xf < yf {
		cols = tbCols
		rows = uint(math.Floor(float64(imRows) * xf))
	} else if yf < xf {
		cols = uint(math.Floor(float64(imCols) * yf))
		rows = tbRows
	} else {
		cols = tbCols
		rows = tbRows
	}

	if imCols*imRows > SamplePixel {
		log.Printf("%dx%d > sample pixel, sampling...", imCols, imRows)
		return im.SampleImage(cols, rows)
	}
	return im.ResizeImage(cols, rows, imagick.FILTER_LANCZOS, 0.8)
}

func ImFitPixelResize(im *imagick.MagickWand, tb common.Thumb) error {
	// log.Printf("fit pixel resizing...")
	// start := time.Now()
	imCols := im.GetImageWidth()
	imRows := im.GetImageHeight()
	imPixel := imCols * imRows
	pf := math.Sqrt(float64(tb.Pixels) / float64(imPixel))
	cols := uint(math.Floor(float64(imCols) * pf))
	rows := uint(math.Floor(float64(imRows) * pf))
	var err error
	if imCols*imRows > SamplePixel {
		log.Printf("%dx%d > sample pixel, sampling...", imCols, imRows)
		err = im.SampleImage(cols, rows)
	} else {
		err = im.ResizeImage(cols, rows, imagick.FILTER_LANCZOS, 0.8)
	}
	if err != nil {
		return err
	}
	// log.Printf("pixel resize resize image: %v", time.Since(start))
	return nil
}
