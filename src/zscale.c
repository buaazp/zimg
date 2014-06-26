#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include "webimg/webimg2.h"
#include "zlog.h"
#include "zcommon.h"
#include "zscale.h"

static int proportion(struct image *im, int p_type, uint32_t cols, uint32_t rows);
static int crop(struct image *im,  uint32_t x, uint32_t y, uint32_t cols, uint32_t rows);
int convert(struct image *im, zimg_req_t *req);


/*
 * 
 * sample URLs:
 * http://127.0.0.1:4869/md5?w=300
 * http://127.0.0.1:4869/md5?w=200&h=300&p=0
 * http://127.0.0.1:4869/md5?w=200&h=300&p=2
 * http://127.0.0.1:4869/md5?x=0&y=0&w=100&h=100
 * http://127.0.0.1:4869/md5?t=square
 * http://127.0.0.1:4869/md5?t=maxwidth
 * http://127.0.0.1:4869/md5?t=maxsize
 *
 *
 *
 */

static int square(struct image *im, uint32_t size)
{
	int ret;
	uint32_t x, y, cols;

	if (im->cols > im->rows) {
		cols = im->rows;
		y = 0;
		x = (uint32_t)floor((im->cols - im->rows) / 2.0);
	} else {
		cols = im->cols;
		x = 0;
		y = (uint32_t)floor((im->rows - im->cols) / 2.0);
	}

	ret = wi_crop(im, x, y, cols, cols);
	if (ret != WI_OK) return -1;

	ret = wi_scale(im, size, 0);
	if (ret != WI_OK) return -1;

	return 0;
}

static int proportion(struct image *im, int p_type, uint32_t cols, uint32_t rows)
{
	int ret;

    if(p_type == 1)
    {
        if (cols > 0)
        {
            rows = round(((double)cols / im->cols) * im->rows);
        }
        else
        {
            cols = round(((double)rows / im->rows) * im->cols);
        }
        LOG_PRINT(LOG_DEBUG, "p=1, wi_scale(im, %d, %d)", cols, rows);
        ret = wi_scale(im, cols, rows);
    }
    else if (p_type == 0)
    {
        if (cols == 0) cols = im->cols;
        if (rows == 0) rows = im->rows;
        LOG_PRINT(LOG_DEBUG, "p=0, wi_scale(im, %d, %d)", cols, rows);
        ret = wi_scale(im, cols, rows);
    }
    else if (p_type == 2)
    {
        uint32_t x, y;
        if (cols == 0) cols = im->cols;
        if (rows == 0) rows = im->rows;
        x = (uint32_t)floor((im->cols - cols) / 2.0);
        y = (uint32_t)floor((im->rows - rows) / 2.0);
        LOG_PRINT(LOG_DEBUG, "p=2, wi_crop(im, %d, %d, %d, %d)", x, y, cols, rows);
        ret = wi_crop(im, x, y, cols, rows);
    }

	return ret;
}

static int crop(struct image *im,  uint32_t x, uint32_t y, uint32_t cols, uint32_t rows)
{
    int ret;
    if (x >= im->cols || y >= im->rows) return -1;
    if (cols == 0 || im->cols < x + cols) cols = im->cols - x;
    if (rows == 0 || im->rows < y + rows) rows = im->rows - y;
    LOG_PRINT(LOG_DEBUG, "wi_crop(im, %d, %d, %d, %d)", x, y, cols, rows);
    ret = wi_crop(im, x, y, cols, rows);
    return ret;
}

int convert(struct image *im, zimg_req_t *req)
{
    int result = 0, ret;
    uint32_t x = req->x, y = req->y, cols = req->width, rows = req->height;
    if (cols == 0 && rows == 0) goto grayscale;
    if (im->cols < cols || im->rows < rows) {
        result = 1;
        goto grayscale;
    }

    /* crop and scale */
    if (x == 0 && y == 0) {
        LOG_PRINT(LOG_DEBUG, "proportion(im, %d, %d, %d)", req->proportion, cols, rows);
        ret = proportion(im, req->proportion, cols, rows);
        if (ret != WI_OK) return -1;
    } else {
        LOG_PRINT(LOG_DEBUG, "crop(im, %d, %d, %d, %d)", x, y, cols, rows);
        ret = crop(im, x, y, cols, rows);
        if (ret != WI_OK) return -1;
    }

grayscale:
    /* set gray */
    if (req->gray && im->colorspace != CS_GRAYSCALE) {
        LOG_PRINT(LOG_DEBUG, "wi_gray(im)");
        ret = wi_gray(im);
        if (ret != WI_OK) return -1;
    }

	/* set quality */
	if (im->quality > WAP_QUALITY) {
        LOG_PRINT(LOG_DEBUG, "wi_set_quality(im, %d)", WAP_QUALITY);
		wi_set_quality(im, WAP_QUALITY);
	}

	/* set format */
	if (strncmp(im->format, "GIF", 3) != 0) {
        LOG_PRINT(LOG_DEBUG, "wi_set_format(im, %s)", "JPEG");
        ret = wi_set_format(im, "JPEG");
        if (ret != WI_OK) return -1;
	}

    LOG_PRINT(LOG_DEBUG, "convert(im, req) %d", result);
	return result;
}

