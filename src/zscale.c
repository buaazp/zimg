/*
 *   zimg - high performance image storage and processing system.
 *       http://zimg.buaa.us
 *
 *   Copyright (c) 2013-2014, Peter Zhao <zp@buaa.us>.
 *   All rights reserved.
 *
 *   Use and distribution licensed under the BSD license.
 *   See the LICENSE file for full text.
 *
 */

/**
 * @file zscale.c
 * @brief scale image functions by graphicsmagick.
 * @author 招牌疯子 zp@buaa.us
 * @version 3.0.0
 * @date 2014-08-14
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <wand/magick_wand.h>
#include "zlog.h"
#include "zcommon.h"
#include "zscale.h"

static int proportion(MagickWand *im, int p_type, int cols, int rows);
static int crop(MagickWand *im, int x, int y, int cols, int rows);
int convert(MagickWand *im, zimg_req_t *req);

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
static int proportion(MagickWand *im, int p_type, int cols, int rows)
{
    int ret = -1;
    unsigned long im_cols = MagickGetImageWidth(im);
    unsigned long im_rows = MagickGetImageHeight(im);

    if (settings.disable_zoom_up == 1)
    {
        if (p_type == 3)
        {
            cols = cols > 100 ? 100 : cols;
            rows = rows > 100 ? 100 : rows;
        }
        else
        {
            cols = cols > im_cols ? im_cols : cols;
            rows = rows > im_rows ? im_rows : rows;
        }
    }

    if (p_type == 1)
    {
        if (cols == 0 || rows == 0)
        {
            if (cols > 0)
            {
                rows = (uint32_t)round(((double)cols / im_cols) * im_rows);
            }
            else
            {
                cols = (uint32_t)round(((double)rows / im_rows) * im_cols);
            }
            ret = MagickResizeImage(im, cols, rows, LanczosFilter, 1.0);
            LOG_PRINT(LOG_DEBUG, "p=1, wi_scale(im, %d, %d) ret = %d", cols, rows, ret);
            //ret = MagickScaleImage(im, cols, rows);
        }
        else
        {
            uint32_t x = 0, y = 0, s_cols, s_rows;
            double cols_rate = (double)cols / im_cols;
            double rows_rate = (double)rows / im_rows;

            if (cols_rate > rows_rate)
            {
                s_cols = cols;
                s_rows = (uint32_t)round(cols_rate * im_rows);
                y = (uint32_t)floor((s_rows - rows) / 2.0);
            }
            else
            {
                s_cols = (uint32_t)round(rows_rate * im_cols);
                s_rows = rows;
                x = (uint32_t)floor((s_cols - cols) / 2.0);
            }
            ret = MagickResizeImage(im, s_cols, s_rows, LanczosFilter, 1.0);
            LOG_PRINT(LOG_DEBUG, "p=2, wi_scale(im, %d, %d) ret = %d", s_cols, s_rows, ret);
            //ret = MagickScaleImage(im, s_cols, s_rows);

            ret = MagickCropImage(im, cols, rows, x, y);
            LOG_PRINT(LOG_DEBUG, "p=2, wi_crop(im, %d, %d, %d, %d) ret = %d", x, y, cols, rows, ret);
        }
    }
    else if (p_type == 2)
    {
        uint32_t x, y;
        x = (uint32_t)floor((im_cols - cols) / 2.0);
        y = (uint32_t)floor((im_rows - rows) / 2.0);
        ret = MagickCropImage(im, cols, rows, x, y);
        LOG_PRINT(LOG_DEBUG, "p=2, wi_crop(im, %d, %d, %d, %d) ret = %d", x, y, cols, rows, ret);
    }
    else if (p_type == 3)
    {
        if (cols == 0 || rows == 0)
        {
            int rate = cols > 0 ? cols : rows;
            rows = (uint32_t)round(im_rows * (double)rate / 100);
            cols = (uint32_t)round(im_cols * (double)rate / 100);
            ret = MagickResizeImage(im, cols, rows, LanczosFilter, 1.0);
            LOG_PRINT(LOG_DEBUG, "p=3, wi_scale(im, %d, %d) ret = %d", cols, rows, ret);
            //ret = MagickScaleImage(im, cols, rows);
        }
        else
        {
            rows = (uint32_t)round(im_rows * (double)rows / 100);
            cols = (uint32_t)round(im_cols * (double)cols / 100);
            ret = MagickResizeImage(im, cols, rows, LanczosFilter, 1.0);
            LOG_PRINT(LOG_DEBUG, "p=3, wi_scale(im, %d, %d) ret = %d", cols, rows, ret);
            //ret = MagickScaleImage(im, cols, rows);
        }
    }
    else if (p_type == 0)
    {
        ret = MagickResizeImage(im, cols, rows, LanczosFilter, 1.0);
        LOG_PRINT(LOG_DEBUG, "p=0, wi_scale(im, %d, %d) ret = %d", cols, rows, ret);
        //ret = MagickScaleImage(im, cols, rows);
    }
    else if (p_type == 4)
    {
        double rate = 1.0;
        if (cols == 0 || rows == 0)
        {
            rate = cols > 0 ? (double)cols / im_cols : (double)rows / im_rows;
        }
        else
        {
            double rate_col = (double)cols / im_cols;
            double rate_row = (double)rows / im_rows;
            rate = rate_col < rate_row ? rate_col : rate_row;
        }
        cols = (uint32_t)round(im_cols * rate);
        rows = (uint32_t)round(im_rows * rate);
        ret = MagickResizeImage(im, cols, rows, LanczosFilter, 1.0);
        LOG_PRINT(LOG_DEBUG, "p=4, wi_scale(im, %d, %d) ret = %d", cols, rows, ret);
    }

    return ret;
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
static int crop(MagickWand *im, int x, int y, int cols, int rows)
{
    int ret = -1;
    unsigned long im_cols = MagickGetImageWidth(im);
    unsigned long im_rows = MagickGetImageHeight(im);
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= im_cols || y >= im_rows) return -1;
    if (cols == 0 || im_cols < x + cols) cols = im_cols - x;
    if (rows == 0 || im_rows < y + rows) rows = im_rows - y;
    ret = MagickCropImage(im, cols, rows, x, y);
    LOG_PRINT(LOG_DEBUG, "wi_crop(im, %d, %d, %d, %d) ret = %d", x, y, cols, rows, ret);
    return ret;
}

/**
 * @brief convert convert image function
 *
 * @param im the image
 * @param req the zimg request
 *
 * @return 1 for OK and -1 for fail
 */
int convert(MagickWand *im, zimg_req_t *req)
{
    int result = 1, ret = -1;

    MagickResetIterator(im);

    int orientation = MagickGetImageOrientation(im);
    if (orientation > 1)
    {
        ret = MagickAutoOrientImage(im);
        LOG_PRINT(LOG_DEBUG, "orientation: %d auto_orientat() ret = %d", orientation, ret);
        if (ret != MagickTrue) return -1;
    }

    int x = req->x, y = req->y, cols = req->width, rows = req->height;
    if (!(cols == 0 && rows == 0)) {
        /* crop and scale */
        if (x == -1 && y == -1) {
            ret = proportion(im, req->proportion, cols, rows);
            LOG_PRINT(LOG_DEBUG, "proportion(im, %d, %d, %d) ret = %d", req->proportion, cols, rows, ret);
            if (ret != MagickTrue) return -1;
        } else {
            ret = crop(im, x, y, cols, rows);
            LOG_PRINT(LOG_DEBUG, "crop(im, %d, %d, %d, %d) ret = %d", x, y, cols, rows, ret);
            if (ret != MagickTrue) return -1;
        }
    }

    /* rotate image */
    if (req->rotate != 0) {
        PixelWand *background = NewPixelWand();
        if (background == NULL) return -1;
        ret = PixelSetColor(background, "white");
        LOG_PRINT(LOG_DEBUG, "pixelwand setcolor() ret = %d", ret);
        if (ret != MagickTrue) {
            DestroyPixelWand(background);
            return -1;
        }
        ret = MagickRotateImage(im, background, req->rotate);
        LOG_PRINT(LOG_DEBUG, "wi_rotate(im, %d) ret = %d", req->rotate, ret);
        DestroyPixelWand(background);
        if (ret != MagickTrue) return -1;
    }

    /* set gray */
    if (req->gray == 1) {
        LOG_PRINT(LOG_DEBUG, "wi_gray(im)");
        //several ways to grayscale an image:
        //ret = MagickSetImageColorspace(im, GRAYColorspace);
        //ret = MagickQuantizeImage(im, 256, GRAYColorspace, 0, MagickFalse, MagickFalse);
        //ret = MagickSeparateImageChannel(im, GrayChannel);
        ret = MagickSetImageType(im, GrayscaleType);
        LOG_PRINT(LOG_DEBUG, "gray() ret = %d", ret);
        if (ret != MagickTrue) return -1;
    }

    /* set quality */
    int quality = MagickGetImageCompressionQuality(im);
    if (quality == 0 || quality > req->quality)
    {
        ret = MagickSetImageCompressionQuality(im, req->quality);
        LOG_PRINT(LOG_DEBUG, "wi_set_quality(im, %d) ret = %d", req->quality, ret);
        if (ret != MagickTrue) return -1;
    }

    /* set format */
    if (strncmp(req->fmt, "none", 4) != 0) {
        ret = MagickSetImageFormat(im, req->fmt);
        LOG_PRINT(LOG_DEBUG, "wi_set_format(im, %s) ret = %d", req->fmt, ret);
        if (ret != MagickTrue) return -1;
    }

    /* strip image metadata */
    ret = MagickStripImage(im);
    LOG_PRINT(LOG_DEBUG, "strip_image() ret = %d", ret);
    if (ret != MagickTrue) return -1;

    LOG_PRINT(LOG_DEBUG, "convert(im, req) %d", result);
    return result;
}

