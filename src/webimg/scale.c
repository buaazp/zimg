#include <math.h>
#include <string.h>
#include <assert.h>
#include "webimg2.h"
#include "internal.h"

static void calc_sample_table(uint32_t *table, uint32_t s, uint32_t d)
{
	uint32_t *ptr = table;
	int i;
	double span, scale;

	scale = s / (double)d;
	span = scale;

	for (i=0; i<d; i++) {
		ptr[i] = (int)span;
		span += scale;
	}

	return;
}

static inline void scale_box_rgb_first_line
(uint32_t *line, uint8_t *src, uint32_t *tbl, uint32_t d)
{
	int i, j;
	uint32_t r, g, b;
	uint32_t *tp = line;
	uint8_t *sp = src;

	j = 0;
	for (i=0; i<d; i++) {
		/* x a pix */
		r = *sp++;
		g = *sp++;
		b = *sp++;
		j++;
		for (; j<tbl[i]; j++) {
			r += *sp++;
			g += *sp++;
			b += *sp++;
		}
		*tp++ = r;
		*tp++ = g;
		*tp++ = b;
	}
}

static inline void scale_box_rgb_line
(uint32_t *line, uint8_t *src, uint32_t *tbl, uint32_t d)
{
	int i, j;
	uint32_t r, g, b;
	uint32_t *tp = line;
	uint8_t *sp = src;

	j = 0;
	for (i=0; i<d; i++) {
		/* x a pix */
		r = *sp++;
		g = *sp++;
		b = *sp++;
		j++;
		for (; j<tbl[i]; j++) {
			r += *sp++;
			g += *sp++;
			b += *sp++;
		}
		*tp++ += r;
		*tp++ += g;
		*tp++ += b;
	}
}

/* standard */
int scaledown_box_rgb(struct image *src, struct image *dst)
{
	uint32_t *tmpline;
	uint32_t * xtbl, *ytbl;

	xtbl = MALLOC(dst, sizeof(uint32_t) * dst->cols);
	ytbl = MALLOC(dst, sizeof(uint32_t) * dst->rows);
	if (xtbl == NULL) return -1;
	if (xtbl == NULL) {
		FREE(dst, xtbl);
		return -1;
	}
	tmpline = MALLOC(dst, sizeof(uint32_t) * dst->cols * 3);
	if (tmpline == NULL) {
		FREE(dst, xtbl);
		FREE(dst, ytbl);
		return -1;
	}

	calc_sample_table(xtbl, src->cols, dst->cols);
	calc_sample_table(ytbl, src->rows, dst->rows);

	uint8_t *dp;
	uint32_t *tp;
	int i, j = 0, k;
	int xs, ys = 0;
	int span, xspan, yspan = 0;

	for (i=0; i<dst->rows; i++) {
		scale_box_rgb_first_line(tmpline, src->row[j++], xtbl, dst->cols);
		for (; j<ytbl[i]; j++) {
			scale_box_rgb_line(tmpline, src->row[j], xtbl, dst->cols);
		}

		dp = dst->row[i];
		tp = tmpline;

		yspan = ytbl[i] - ys;
		ys = ytbl[i];

		xs = 0;
		xspan = 0;

		for (k=0; k<dst->cols; k++) {
			xspan = xtbl[k] - xs;
			xs = xtbl[k];

			span = xspan * yspan;
			if (span == 0) span = 1;

			*dp++ = *tp++ / span;
			*dp++ = *tp++ / span;
			*dp++ = *tp++ / span;
		}
	}

	FREE(dst, xtbl);
	FREE(dst, ytbl);
	FREE(dst, tmpline);
	return 0;
}

static inline void scale_box_rgba_first_line
(uint32_t *line, uint8_t *src, uint32_t *tbl, uint32_t d)
{
	int i, j;
	uint32_t r, g, b, a;
	uint32_t *tp = line;
	uint8_t *sp = src;

	j = 0;
	for (i=0; i<d; i++) {
		/* x a pix */
		r = *sp++;
		g = *sp++;
		b = *sp++;
		a = *sp++;
		j++;
		for (; j<tbl[i]; j++) {
			r += *sp++;
			g += *sp++;
			b += *sp++;
			a += *sp++;
		}
		*tp++ = r;
		*tp++ = g;
		*tp++ = b;
		*tp++ = a;
	}
}

static inline void scale_box_rgba_line
(uint32_t *line, uint8_t *src, uint32_t *tbl, uint32_t d)
{
	int i, j;
	uint32_t r, g, b, a;
	uint32_t *tp = line;
	uint8_t *sp = src;

	j = 0;
	for (i=0; i<d; i++) {
		/* x a pix */
		r = *sp++;
		g = *sp++;
		b = *sp++;
		a = *sp++;
		j++;
		for (; j<tbl[i]; j++) {
			r += *sp++;
			g += *sp++;
			b += *sp++;
			a += *sp++;
		}
		*tp++ += r;
		*tp++ += g;
		*tp++ += b;
		*tp++ += a;
	}
}

/* standard */
int scaledown_box_rgba(struct image *src, struct image *dst)
{
	uint32_t *tmpline;
	uint32_t * xtbl, *ytbl;

	xtbl = MALLOC(dst, sizeof(uint32_t) * dst->cols);
	ytbl = MALLOC(dst, sizeof(uint32_t) * dst->rows);
	if (xtbl == NULL) return -1;
	if (xtbl == NULL) {
		FREE(dst, xtbl);
		return -1;
	}
	tmpline = MALLOC(dst, sizeof(uint32_t) * dst->cols * 3);
	if (tmpline == NULL) {
		FREE(dst, xtbl);
		FREE(dst, ytbl);
		return -1;
	}

	calc_sample_table(xtbl, src->cols, dst->cols);
	calc_sample_table(ytbl, src->rows, dst->rows);

	uint8_t *dp;
	uint32_t *tp;
	int i, j = 0, k;
	int xs, ys = 0;
	int span, xspan, yspan = 0;

	for (i=0; i<dst->rows; i++) {
		scale_box_rgba_first_line(tmpline, src->row[j++], xtbl, dst->cols);
		for (; j<ytbl[i]; j++) {
			scale_box_rgba_line(tmpline, src->row[j], xtbl, dst->cols);
		}

		dp = dst->row[i];
		tp = tmpline;

		yspan = ytbl[i] - ys;
		ys = ytbl[i];

		xs = 0;
		xspan = 0;
		for (k=0; k<dst->cols; k++) {
			xspan = xtbl[k] - xs;
			xs = xtbl[k];

			span = xspan * yspan;
			if (span == 0) span = 1;

			*dp++ = *tp++ / span;
			*dp++ = *tp++ / span;
			*dp++ = *tp++ / span;
			*dp++ = *tp++ / span;
		}
	}

	FREE(dst, xtbl);
	FREE(dst, ytbl);
	FREE(dst, tmpline);
	return 0;
}

static inline void scale_box_gray_first_line
(uint32_t *line, uint8_t *src, uint32_t *tbl, uint32_t d)
{
	int i, j;
	uint32_t l;
	uint32_t *tp = line;
	uint8_t *sp = src;

	j = 0;
	for (i=0; i<d; i++) {
		/* x a pix */
		l = *sp++;
		j++;
		for (; j<tbl[i]; j++) {
			l += *sp++;
		}
		*tp++ = l;
	}
}

static inline void scale_box_gray_line
(uint32_t *line, uint8_t *src, uint32_t *tbl, uint32_t d)
{
	int i, j;
	uint32_t l;
	uint32_t *tp = line;
	uint8_t *sp = src;

	j = 0;
	for (i=0; i<d; i++) {
		/* x a pix */
		l = *sp++;
		j++;
		for (; j<tbl[i]; j++) {
			l += *sp++;
		}
		*tp++ += l;
	}
}

/* standard */
int scaledown_box_gray(struct image *src, struct image *dst)
{
	uint32_t *tmpline;
	uint32_t * xtbl, *ytbl;

	xtbl = MALLOC(dst, sizeof(uint32_t) * dst->cols);
	ytbl = MALLOC(dst, sizeof(uint32_t) * dst->rows);
	if (xtbl == NULL) return -1;
	if (xtbl == NULL) {
		FREE(dst, xtbl);
		return -1;
	}
	tmpline = MALLOC(dst, sizeof(uint32_t) * dst->cols * 3);
	if (tmpline == NULL) {
		FREE(dst, xtbl);
		FREE(dst, ytbl);
		return -1;
	}

	calc_sample_table(xtbl, src->cols, dst->cols);
	calc_sample_table(ytbl, src->rows, dst->rows);

	uint8_t *dp;
	uint32_t *tp;
	int i, j = 0, k;
	int xs, ys = 0;
	int span, xspan, yspan = 0;

	for (i=0; i<dst->rows; i++) {
		scale_box_gray_first_line(tmpline, src->row[j++], xtbl, dst->cols);
		for (; j<ytbl[i]; j++) {
			scale_box_gray_line(tmpline, src->row[j], xtbl, dst->cols);
		}

		dp = dst->row[i];
		tp = tmpline;

		yspan = ytbl[i] - ys;
		ys = ytbl[i];

		xs = 0;
		xspan = 0;

		for (k=0; k<dst->cols; k++) {
			xspan = xtbl[k] - xs;
			xs = xtbl[k];

			span = xspan * yspan;
			if (span == 0) span = 1;

			*dp++ = *tp++ / span;
		}
	}

	FREE(dst, xtbl);
	FREE(dst, ytbl);
	FREE(dst, tmpline);
	return 0;
}

int wi_scale(struct image *im, uint32_t cols, uint32_t rows)
{
	int ret = WI_OK;
	struct image dst;

	if (cols == 0 && rows == 0) return -1;
	if (cols > 0 && rows > 0) return -1;

	if (cols > 0) {
		rows = round(((double)cols / im->cols) * im->rows);
		if (cols >= im->cols) return 0;
	} else {
		cols = round(((double)rows / im->rows) * im->cols);
		if (rows >= im->rows) return 0;
	}

	/* pre resample */
#ifdef PRERESAMPLE
	int scale = im->cols / cols;
	if (scale >= 2) {
		ret = presample(im, 4);
	} else if (scale >= 4) {
		ret = presample(im, 2);
	} else if (scale >= 8) {
		ret = presample(im, 1);
	}
	if (ret != WI_OK) return WI_E_LOADER_LOAD;
#endif

	ret = load_image(im);
	if (ret != WI_OK) return WI_E_LOADER_LOAD;

	memcpy(&dst, im, sizeof(struct image));
	dst.cols = cols;
	dst.rows = rows;

	ret = image_alloc_data(&dst);
	if (ret != 0) return -1;

	if (im->colorspace == CS_RGB) {
		ret = scaledown_box_rgb(im, &dst);
	} else if (im->colorspace == CS_GRAYSCALE) {
		ret = scaledown_box_gray(im, &dst);
	} else if (im->colorspace == CS_RGBA) {
		ret = scaledown_box_rgba(im, &dst);
	} else {
		image_free_data(&dst);
		return -1;
	}

	if (ret != 0) {
		image_free_data(&dst);
		return -1;
	}

	image_free_data(im);
	memcpy(im, &dst, sizeof(struct image));

	im->converted = 1;

	return 0;
}
