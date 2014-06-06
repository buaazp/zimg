#include <assert.h>
#include <string.h>
#include <webp/decode.h>
#include <webp/encode.h>
#include <webp/types.h>

#include "webimg2.h"
#include "internal.h"

#define WI_WEBP_MAGIC		"RIFF"
#define WI_WEBP_MAGIC_LEN	4
#define CHECK_WEBP_BY_MAGIC

int wi_is_webp(void *data, size_t len)
{
#ifndef CHECK_WEBP_BY_MAGIC
	int ret;
	int w, h;

	ret = WebPGetInfo(data, len, &w, &h);
	if (ret != VP8_STATUS_OK) return 0;

	if (w > WEBP_MAX_DIMENSION || h > WEBP_MAX_DIMENSION) {
		return 0;
	}
#else
	if (memcmp(data, WI_WEBP_MAGIC, WI_WEBP_MAGIC_LEN) != 0) {
		return 0;
	}
#endif

	return 1;
}

/* read file into input_blob */
int wi_webp_init_read(struct image *im)
{
	return 0;
}

int wi_webp_read_img_info(struct image *im)
{
	VP8StatusCode ret;
	WebPBitstreamFeatures info;

	ret = WebPGetFeatures(im->in_buf.data, im->in_buf.len, &info);
	if (ret == VP8_STATUS_OK) {
		im->rows = info.height;
		im->cols = info.width;
		im->has_alpha = info.has_alpha;
		return 0;
	}

	return -1;
}

void wi_webp_resample(struct image *im, int num, int demon)
{
	return;
}

int wi_webp_load_image(struct image *im)
{
	int ret;
	uint8_t *data;
	
	im->colorspace = CS_RGB;

	ret = image_alloc_data(im);
	if (ret != 0) return -1;

	if (im->has_alpha) {
		data = WebPDecodeRGBAInto((const uint8_t *)im->in_buf.data, im->in_buf.len,
				im->data, im->rows * im->cols * 4, im->cols * 4);
	} else {
		data = WebPDecodeRGBInto((const uint8_t *)im->in_buf.data, im->in_buf.len,
				im->data, im->rows * im->cols * 3, im->cols * 3);
	}

	if (data == NULL) {
		FREE(im, im->row);
		im->row = NULL;
		im->data = NULL;
		return -1;
	}

	im->loaded = 1;

	return 0;
}

void wi_webp_finish_read(struct image *im)
{
	return;
}

static ssize_t wi_webp_encode_gray(struct image *im, uint8_t **data)
{
	int i;
	size_t size;
	uint32_t pixs;
	uint8_t *src, *p;

	pixs = im->rows * im->cols;

	p = src = MALLOC(im, pixs * 3);
	if (src == NULL) return -1;

	/* map gray to rgb package, since webp dose not support GRAYSCALE */
	for (i=0; i<pixs; i++) {
		*p++ = im->data[i];
		*p++ = im->data[i];
		*p++ = im->data[i];
	}

	size = WebPEncodeRGB(src, im->cols, im->rows,
			im->cols*3, (float)im->quality, data);
	FREE(im, src);

	return size;
}

int wi_webp_save(struct image *im)
{
	int ret;
	ssize_t size;
	uint8_t *data;

	if (im->colorspace == CS_RGB) {
		size = WebPEncodeRGB(im->data, im->cols, im->rows,
				im->cols*3, (float)im->quality, &data);
	} else if (im->colorspace == CS_GRAYSCALE) {
		size = wi_webp_encode_gray(im, &data);
	} else if (im->colorspace == CS_RGBA) {
		size = WebPEncodeRGBA(im->data, im->cols, im->rows,
				im->cols*4, (float)im->quality, &data);
	} else {
		return -1;
	}

	if (size < 0) return -1;

	ret = expand_blob(im, &im->ou_buf, size);
	if (ret != 0) {
		/* alloced by libwebp */
		free(data);
		return -1;
	}

	memcpy(im->ou_buf.data, data, size);
	free(data);

	return 0;
}

struct loader webp_loader = {
	.name			= "WEBP",
	.ext			= ".webp",
	.mime_type		= "image/webp",

	.suitable		= wi_is_webp,
	.init			= NULL,
	.read_info		= wi_webp_read_img_info,
	.resample		= NULL,
	.load			= wi_webp_load_image,
	.save			= wi_webp_save,
	.finish			= wi_webp_finish_read,
};
