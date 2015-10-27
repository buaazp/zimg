/*
 *   zimg - high performance image storage and processing system.
 *       http://zimg.buaa.us
 *
 *   Copyright (c) 2013-2015, Peter Zhao <zp@buaa.us>.
 *   All rights reserved.
 *
 *   Use and distribution licensed under the BSD license.
 *   See the LICENSE file for full text.
 *
 */

/**
 * @file zimage.c
 * @brief Image convert interface.
 * @author 招牌疯子 zp@buaa.us
 * @version 3.2.0
 * @date 2015-10-24
 */

#include "zrestful.h"
#include "zimage.h"
#include "zlog.h"

static int proportion(MagickWand *im, int p_type, int cols, int rows);
static int crop(MagickWand *im, int x, int y, int cols, int rows);
int convert2(zreq_t *zreq);

/**
 * @brief proportion proportion function
 *
 * @param im the image
 * @param p_type p type
 * @param cols width of target image
 * @param rows height of target image
 *
 * @return 0 for OK and -1 for fail
 */
static int proportion(MagickWand *im, int p_type, int cols, int rows) {
    MagickBooleanType ret = MagickFalse;
    unsigned long im_cols = MagickGetImageWidth(im);
    unsigned long im_rows = MagickGetImageHeight(im);

    if (settings.disable_zoom_up == 1) {
        if (p_type == 3) {
            cols = cols > 100 ? 100 : cols;
            rows = rows > 100 ? 100 : rows;
        } else {
            cols = cols > im_cols ? im_cols : cols;
            rows = rows > im_rows ? im_rows : rows;
        }
    }

    if (p_type == 1) {
        if (cols == 0 || rows == 0) {
            if (cols > 0) {
                rows = (uint32_t)round(((double)cols / im_cols) * im_rows);
            } else {
                cols = (uint32_t)round(((double)rows / im_rows) * im_cols);
            }
            LOG_PRINT(LOG_DEBUG, "p=1, wi_scale(im, %d, %d)", cols, rows);
            ret = MagickResizeImage(im, cols, rows, LanczosFilter, 0.8);
        } else {
            uint32_t x = 0, y = 0, s_cols, s_rows;
            double cols_rate = (double)cols / im_cols;
            double rows_rate = (double)rows / im_rows;

            if (cols_rate > rows_rate) {
                s_cols = cols;
                s_rows = (uint32_t)round(cols_rate * im_rows);
                y = (uint32_t)floor((s_rows - rows) / 2.0);
            } else {
                s_cols = (uint32_t)round(rows_rate * im_cols);
                s_rows = rows;
                x = (uint32_t)floor((s_cols - cols) / 2.0);
            }
            LOG_PRINT(LOG_DEBUG, "p=2, wi_scale(im, %d, %d)", s_cols, s_rows);
            ret = MagickResizeImage(im, s_cols, s_rows, LanczosFilter, 0.8);

            LOG_PRINT(LOG_DEBUG, "p=2, wi_crop(im, %d, %d, %d, %d)", x, y, cols, rows);
            ret = MagickCropImage(im, cols, rows, x, y);
        }
    } else if (p_type == 2) {
        uint32_t x, y;
        x = (uint32_t)floor((im_cols - cols) / 2.0);
        y = (uint32_t)floor((im_rows - rows) / 2.0);
        LOG_PRINT(LOG_DEBUG, "p=2, wi_crop(im, %d, %d, %d, %d)", x, y, cols, rows);
        ret = MagickCropImage(im, cols, rows, x, y);
    } else if (p_type == 3) {
        if (cols == 0 || rows == 0) {
            int rate = cols > 0 ? cols : rows;
            rows = (uint32_t)round(im_rows * (double)rate / 100);
            cols = (uint32_t)round(im_cols * (double)rate / 100);
        } else {
            rows = (uint32_t)round(im_rows * (double)rows / 100);
            cols = (uint32_t)round(im_cols * (double)cols / 100);
        }
        if ((settings.max_pixel > 0) &&
                (rows > settings.max_pixel ||
                 cols > settings.max_pixel)) {
            LOG_PRINT(LOG_ERROR, "p=3, resize(%d, %d) max_pixel: %d",
                      cols, rows, settings.max_pixel);
            return api_err_pixel_over;
        }
        LOG_PRINT(LOG_DEBUG, "p=3, wi_scale(im, %d, %d)", cols, rows);
        ret = MagickResizeImage(im, cols, rows, LanczosFilter, 0.8);
    } else if (p_type == 0) {
        LOG_PRINT(LOG_DEBUG, "p=0, wi_scale(im, %d, %d)", cols, rows);
        ret = MagickResizeImage(im, cols, rows, LanczosFilter, 0.8);
    } else if (p_type == 4) {
        double rate = 1.0;
        if (cols == 0 || rows == 0) {
            rate = cols > 0 ? (double)cols / im_cols : (double)rows / im_rows;
        } else {
            double rate_col = (double)cols / im_cols;
            double rate_row = (double)rows / im_rows;
            rate = rate_col < rate_row ? rate_col : rate_row;
        }
        cols = (uint32_t)round(im_cols * rate);
        rows = (uint32_t)round(im_rows * rate);
        LOG_PRINT(LOG_DEBUG, "p=4, wi_scale(im, %d, %d)", cols, rows);
        ret = MagickResizeImage(im, cols, rows, LanczosFilter, 0.8);
    }

    if (ret != MagickTrue) {
        return api_err_imagick;
    }
    return api_ok;
}

/**
 * @brief crop crop an image
 *
 * @param im the image
 * @param x position x
 * @param y position y
 * @param cols target width
 * @param rows target height
 *
 * @return 0 for OK and -1 for fail
 */
static int crop(MagickWand * im, int x, int y, int cols, int rows) {
    MagickBooleanType ret = MagickFalse;
    unsigned long im_cols = MagickGetImageWidth(im);
    unsigned long im_rows = MagickGetImageHeight(im);
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= im_cols || y >= im_rows) return api_err_pixel_over;
    if (cols == 0 || im_cols < x + cols) cols = im_cols - x;
    if (rows == 0 || im_rows < y + rows) rows = im_rows - y;
    LOG_PRINT(LOG_DEBUG, "wi_crop(im, %d, %d, %d, %d)", x, y, cols, rows);
    ret = MagickCropImage(im, cols, rows, x, y);
    if (ret != MagickTrue) {
        return api_err_imagick;
    }
    return api_ok;
}

int convert2(zreq_t *zreq) {
    MagickWand *im = NewMagickWand();
    if (im == NULL) return api_err_imagick;

    MagickBooleanType ret = MagickReadImageBlob(im,
                            (const unsigned char *)zreq->buff_in->buff,
                            zreq->buff_in->len);
    if (ret != MagickTrue) {
        LOG_PRINT(LOG_ERROR, "%s magick read image blob failed", zreq->key);
        DestroyMagickWand(im);
        return api_err_imagick;
    }

    char *format = MagickGetImageFormat(im);
    LOG_PRINT(LOG_DEBUG, "format: %s", format);
    if (strncmp(format, "GIF", 3) == 0) {
        // Composites a set of images while respecting any page
        // offsets and disposal methods
        LOG_PRINT(LOG_DEBUG, "coalesce_images()");
        MagickWand *gifim = MagickCoalesceImages(im);
        if (gifim == NULL) {
            MagickRelinquishMemory(format);
            DestroyMagickWand(im);
            return api_err_imagick;
        }
        DestroyMagickWand(im);
        im = gifim;
    } else if (strncmp(format, "PNG", 3) == 0) {
        // convert transparent to white background
        LOG_PRINT(LOG_DEBUG, "wi_set_background_color()");
        PixelWand *background = NewPixelWand();
        if (background == NULL) {
            MagickRelinquishMemory(format);
            DestroyMagickWand(im);
            return api_err_imagick;
        }
        ret = PixelSetColor(background, "white");
        LOG_PRINT(LOG_DEBUG, "pixel_set_color() ret = %d", ret);
        if (ret != MagickTrue) {
            DestroyPixelWand(background);
            MagickRelinquishMemory(format);
            DestroyMagickWand(im);
            return api_err_imagick;
        }
        ret = MagickSetImageBackgroundColor(im, background);
        LOG_PRINT(LOG_DEBUG, "set_backgroud_color() ret = %d", ret);
        DestroyPixelWand(background);
        if (ret != MagickTrue) {
            MagickRelinquishMemory(format);
            DestroyMagickWand(im);
            return api_err_imagick;
        }
        MagickWand *pngim = MagickMergeImageLayers(im, FlattenLayer);
        if (pngim == NULL) {
            MagickRelinquishMemory(format);
            DestroyMagickWand(im);
            return api_err_imagick;
        }
        DestroyMagickWand(im);
        im = pngim;
    }
    MagickRelinquishMemory(format);

    int x = zreq->x, y = zreq->y, cols = zreq->width, rows = zreq->height;
    if (!(cols == 0 && rows == 0)) {
        /* crop and scale */
        if (x == -1 && y == -1) {
            LOG_PRINT(LOG_DEBUG, "proportion(im, %d, %d, %d)",
                      zreq->proportion, cols, rows);
            int ecode = proportion(im, zreq->proportion, cols, rows);
            if (ecode != api_ok) {
                DestroyMagickWand(im);
                return ecode;
            }
        } else {
            LOG_PRINT(LOG_DEBUG, "crop(im, %d, %d, %d, %d)", x, y, cols, rows);
            int ecode = crop(im, x, y, cols, rows);
            if (ecode != api_ok) {
                DestroyMagickWand(im);
                return ecode;
            }
        }
    }

    /* set gray */
    if (zreq->gray == 1) {
        LOG_PRINT(LOG_DEBUG, "wi_gray(im)");
        ret = MagickSetImageType(im, GrayscaleType);
        LOG_PRINT(LOG_DEBUG, "gray() ret = %d", ret);
        if (ret != MagickTrue) {
            DestroyMagickWand(im);
            return api_err_imagick;
        }
    }


    if (settings.disable_auto_orient == 0) {
        ret = MagickAutoOrientImage(im);
        if (ret != MagickTrue) {
            DestroyMagickWand(im);
            return api_err_imagick;
        }
    }
    /* rotate image */
    if (zreq->rotate != 0) {
        LOG_PRINT(LOG_DEBUG, "wi_rotate(im, %d)", zreq->rotate);
        PixelWand *background = NewPixelWand();
        if (background == NULL) {
            DestroyMagickWand(im);
            return api_err_imagick;
        }
        ret = PixelSetColor(background, "white");
        if (ret != MagickTrue) {
            DestroyPixelWand(background);
            return api_err_imagick;
        }
        ret = MagickRotateImage(im, background, zreq->rotate);
        LOG_PRINT(LOG_DEBUG, "rotate() ret = %d", ret);
        DestroyPixelWand(background);
        if (ret != MagickTrue) {
            DestroyMagickWand(im);
            return api_err_imagick;
        }
    }

    /* set quality */
    // int quality = 100;
    // int im_quality = MagickGetImageCompressionQuality(im);
    // im_quality = (im_quality == 0 ? 100 : im_quality);
    // LOG_PRINT(LOG_DEBUG, "wi_quality = %d", im_quality);
    // quality = zreq->quality < im_quality ? zreq->quality : im_quality;
    LOG_PRINT(LOG_DEBUG, "wi_set_quality(im, %d)", zreq->quality);
    ret = MagickSetImageCompressionQuality(im, zreq->quality);
    if (ret != MagickTrue) {
        DestroyMagickWand(im);
        return api_err_imagick;
    }

    /* set format */
    LOG_PRINT(LOG_DEBUG, "zreq->fmt: %s", zreq->fmt);
    if (strncmp(zreq->fmt, "none", 4) != 0) {
        if (settings.disable_progressive == 0 &&
                ((strncmp(zreq->fmt, "jpeg", 4) == 0) ||
                 (strncmp(zreq->fmt, "jpg", 3) == 0))) {
            LOG_PRINT(LOG_DEBUG, "convert to progressive jpeg");
            // convert image to progressive jpeg
            // progressive reduce jpeg size incidentally
            // but convert elapsed time increase about 10%
            ret = MagickSetInterlaceScheme(im, PlaneInterlace);
            if (ret != MagickTrue) {
                DestroyMagickWand(im);
                return api_err_imagick;
            }
        }
        LOG_PRINT(LOG_DEBUG, "wi_set_format(im, %s)", zreq->fmt);
        ret = MagickSetImageFormat(im, zreq->fmt);
        if (ret != MagickTrue) {
            DestroyMagickWand(im);
            return api_err_imagick;
        }
    }

    size_t len = 0;
    MagickResetIterator(im);
    char *new_buff = (char *)MagickGetImageBlob(im, &len);
    if (new_buff == NULL) {
        LOG_PRINT(LOG_DEBUG, "%s imagick get image blob failed", zreq->key);
        DestroyMagickWand(im);
        return api_err_imagick;
    }
    LOG_PRINT(LOG_DEBUG, "%s new image size = %d", zreq->key, len);
    zreq->buff_out->len = len;
    zreq->buff_out->buff = new_buff;

    DestroyMagickWand(im);
    LOG_PRINT(LOG_DEBUG, "convert succ");
    return api_ok;
}
