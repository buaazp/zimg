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
 * @brief scale image functions by webimg.
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

static int square(MagickWand *im, uint32_t size);
static int proportion(MagickWand *im, int p_type, uint32_t cols, uint32_t rows);
static int crop(MagickWand *im,  uint32_t x, uint32_t y, uint32_t cols, uint32_t rows);
int convert(MagickWand *im, zimg_req_t *req);

/**
 * @brief square square an image
 *
 * @param im the image
 * @param size square size
 *
 * @return 0 for OK and -1 for fail
 */
static int square(MagickWand *im, uint32_t size)
{
	int ret;
	uint32_t x, y, cols;
    unsigned long im_cols = MagickGetImageWidth(im);
    unsigned long im_rows = MagickGetImageHeight(im);

	if (im_cols > im_rows) {
		cols = im_rows;
		y = 0;
		x = (uint32_t)floor((im_cols - im_rows) / 2.0);
	} else {
		cols = im_cols;
		x = 0;
		y = (uint32_t)floor((im_rows - im_cols) / 2.0);
	}

    if(size > cols) size = cols;

    LOG_PRINT(LOG_DEBUG, "p=4, wi_crop(im, %d, %d, %d, %d)", x, y, cols, cols);
    ret = MagickCropImage(im, cols, cols, x, y);
    LOG_PRINT(LOG_DEBUG, "p=4, wi_crop() ret = %d", ret);
	if (ret != MagickTrue) return -1;

    LOG_PRINT(LOG_DEBUG, "p=4, wi_scale(im, %d, %d)", size, size);
    ret = MagickResizeImage(im, size, size, LanczosFilter, 1.0);
    LOG_PRINT(LOG_DEBUG, "p=4, wi_scale() ret = %d", ret);
	if (ret != MagickTrue) return -1;

	return ret;
}

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
static int proportion(MagickWand *im, int p_type, uint32_t cols, uint32_t rows)
{
	int ret = -1;
    unsigned long im_cols = MagickGetImageWidth(im);
    unsigned long im_rows = MagickGetImageHeight(im);

    if(p_type == 1 || cols == 0 || rows == 0)
    {
        if (p_type == 4)
        {
            cols = cols + rows;
            LOG_PRINT(LOG_DEBUG, "p=4, square(im, %d)", cols);
            ret = square(im, cols);
        }
        else
        {
            if (cols > 0)
            {
                rows = (uint32_t)round(((double)cols / im_cols) * im_rows);
            }
            else
            {
                cols = (uint32_t)round(((double)rows / im_rows) * im_cols);
            }
            LOG_PRINT(LOG_DEBUG, "p=1, wi_scale(im, %d, %d)", cols, rows);
            ret = MagickResizeImage(im, cols, rows, LanczosFilter, 1.0);
        }
    }
    else if (p_type == 2)
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
        LOG_PRINT(LOG_DEBUG, "p=2, wi_scale(im, %d, %d)", s_cols, s_rows);
        ret = MagickResizeImage(im, s_cols, s_rows, LanczosFilter, 1.0);

        LOG_PRINT(LOG_DEBUG, "p=2, wi_crop(im, %d, %d, %d, %d)", x, y, cols, rows);
        ret = MagickCropImage(im, cols, rows, x, y);
    }
    else if (p_type == 0)
    {
        LOG_PRINT(LOG_DEBUG, "p=0, wi_scale(im, %d, %d)", cols, rows);
        ret = MagickResizeImage(im, cols, rows, LanczosFilter, 1.0);
    }
    else if (p_type == 3)
    {
        uint32_t x, y;
        x = (uint32_t)floor((im_cols - cols) / 2.0);
        y = (uint32_t)floor((im_rows - rows) / 2.0);
        LOG_PRINT(LOG_DEBUG, "p=3, wi_crop(im, %d, %d, %d, %d)", x, y, cols, rows);
        ret = MagickCropImage(im, cols, rows, x, y);
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
static int crop(MagickWand *im,  uint32_t x, uint32_t y, uint32_t cols, uint32_t rows)
{
    int ret = -1;
    unsigned long im_cols = MagickGetImageWidth(im);
    unsigned long im_rows = MagickGetImageHeight(im);
    if (x >= im_cols || y >= im_rows) return -1;
    if (cols == 0 || im_cols < x + cols) cols = im_cols - x;
    if (rows == 0 || im_rows < y + rows) rows = im_rows - y;
    LOG_PRINT(LOG_DEBUG, "wi_crop(im, %d, %d, %d, %d)", x, y, cols, rows);
    ret = MagickCropImage(im, cols, rows, x, y);
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
    int result = 0, ret = -1;
    ColorspaceType color_space;
    unsigned long im_cols = MagickGetImageWidth(im);
    unsigned long im_rows = MagickGetImageHeight(im);

    uint32_t x = req->x, y = req->y, cols = req->width, rows = req->height;
    if (cols == 0 && rows == 0) goto grayscale;
    if (im_cols < cols || im_rows < rows) {
        result = 1;
        goto grayscale;
    }

    /* crop and scale */
    if (x == -1 && y == -1) {
        LOG_PRINT(LOG_DEBUG, "proportion(im, %d, %d, %d)", req->proportion, cols, rows);
        ret = proportion(im, req->proportion, cols, rows);
        if (ret != MagickTrue) return -1;
    } else {
        LOG_PRINT(LOG_DEBUG, "crop(im, %d, %d, %d, %d)", x, y, cols, rows);
        ret = crop(im, x, y, cols, rows);
        if (ret != MagickTrue) return -1;
    }

grayscale:
    /* set gray */
    color_space = MagickGetImageColorspace(im);
    if (req->gray && color_space != GRAYColorspace) {
        LOG_PRINT(LOG_DEBUG, "wi_gray(im)");
        ret = MagickSetImageColorspace(im, GRAYColorspace);
        LOG_PRINT(LOG_DEBUG, "gray() ret = %d", ret);
        if (ret != MagickTrue) return -1;
    }

	/* set quality */
    if (req->quality > 0 && req->quality <= 100) {
        LOG_PRINT(LOG_DEBUG, "wi_set_quality(im, %d)", req->quality);
        MagickSetCompressionQuality(im, req->quality);
    } else {
        LOG_PRINT(LOG_DEBUG, "wi_set_quality(im, %d)", settings.quality);
        MagickSetCompressionQuality(im, settings.quality);
    }

	/* set format */
	if (settings.dst_format[0] != '\0') {
        LOG_PRINT(LOG_DEBUG, "wi_set_format(im, %s)", settings.dst_format);
        ret = MagickSetFormat(im, settings.dst_format);
        if (ret != MagickTrue) return -1;
    }

    LOG_PRINT(LOG_DEBUG, "convert(im, req) %d", result);
	return result;
}

