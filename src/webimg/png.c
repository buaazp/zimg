#define PNG_SETJMP_SUPPORTED
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <png.h>

#include "webimg2.h"
#include "internal.h"

#ifndef png_jmpbuf
#  define png_jmpbuf(png_ptr) ((png_ptr)->jmpbuf)
#endif

static png_color_16 white_background = {
	.red = 0xff, .green = 0xff, .blue = 0xff,
};

int wi_is_png(void *data, size_t len)
{
	if (len <= 8) return 0;

#if PNG_LIBPNG_VER_MINOR == 4
	return png_sig_cmp((png_bytep)data, 0, 8);
#else
	return png_check_sig((png_bytep)data, 8);
#endif
}

struct wi_png_arg {
	png_structp png_ptr;
	png_infop	info_ptr;
};

void wi_png_read_fn(png_structp png_ptr, png_bytep data, png_size_t len)
{
	size_t left;
	struct image *im;

	im = (struct image *)png_get_io_ptr(png_ptr);

	left = blob_left(&im->in_buf);
	if (left < len) {
		png_error(png_ptr, "read too much");
		return;
	}
	memcpy(data, im->in_buf.ptr, len);
	im->in_buf.ptr += (size_t)len;
}

void wi_png_error_fn(png_structp png_ptr, png_const_charp error_msg)
{
	siglongjmp(png_jmpbuf(png_ptr), 1);
	return;
}

void wi_png_warning_fn(png_structp png_ptr, png_const_charp war_msg)
{
	return;
}

int wi_png_init_read(struct image *im)
{
	struct wi_png_arg *arg;

	arg = MALLOC(im, sizeof(struct wi_png_arg));
	if (arg == NULL) return -1;

	arg->png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
			NULL, NULL, NULL);
	if (arg->png_ptr == NULL) {
		FREE(im, arg);
		return -1;
	}
	arg->info_ptr = png_create_info_struct(arg->png_ptr);
	if (arg->info_ptr == NULL) {
		FREE(im, arg);
		return -1;
	}

	png_set_error_fn(arg->png_ptr, NULL, wi_png_error_fn, wi_png_warning_fn);
	if (setjmp(png_jmpbuf(arg->png_ptr))) {
		png_destroy_read_struct(&arg->png_ptr, &arg->info_ptr, NULL);
		FREE(im, arg);
		im->load_arg = NULL;
		return -1;
	}

	png_set_read_fn(arg->png_ptr, (png_voidp)im, wi_png_read_fn);

	im->load_arg = arg;

	return 0;
}

int wi_png_read_img_info(struct image *im)
{
	struct wi_png_arg *arg;
	png_uint_32 cols, rows;
	int depth, color, interlace;

	arg = im->load_arg;

	if (setjmp(png_jmpbuf(arg->png_ptr)) ) {
		png_destroy_read_struct(&arg->png_ptr, &arg->info_ptr, NULL);
		FREE(im, arg);
		im->load_arg = NULL;
		return -1;
	}

	png_read_info(arg->png_ptr, arg->info_ptr);
	png_get_IHDR(arg->png_ptr, arg->info_ptr, &cols, &rows,
			&depth, &color, &interlace, NULL, NULL);
	im->cols = (uint32_t)cols;
	im->rows = (uint32_t)rows;

	//im->alpha = 0;
	/*
	if (png_get_valid(arg->png_ptr, arg->info_ptr, PNG_INFO_tRNS)
			|| color == PNG_COLOR_TYPE_GRAY_ALPHA
			|| color == PNG_COLOR_TYPE_RGB_ALPHA) {
		im->alpha = 1;
	}
	*/

	if (color == PNG_COLOR_TYPE_PALETTE) {
		png_set_palette_to_rgb(arg->png_ptr);
	}

	if (color == PNG_COLOR_TYPE_GRAY || color == PNG_COLOR_TYPE_GRAY_ALPHA) {
		png_set_gray_to_rgb(arg->png_ptr);
		if (depth < 8) png_set_expand_gray_1_2_4_to_8(arg->png_ptr);
	}

	if (png_get_valid(arg->png_ptr, arg->info_ptr, PNG_INFO_tRNS)) {
		png_set_tRNS_to_alpha(arg->png_ptr);
	}

	/* strip 16bits depth to 8 */
	if (depth > 8) png_set_strip_16(arg->png_ptr);

	if (color & PNG_COLOR_MASK_ALPHA) {
		if (im->has_alpha == 0) {
			png_set_strip_alpha(arg->png_ptr);
			png_set_background(arg->png_ptr, &white_background,
					PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0);
			im->colorspace = CS_RGB;
		} else {
			im->colorspace = CS_RGBA;
		}
	} else {
		png_set_strip_alpha(arg->png_ptr);
		im->has_alpha = 0;
		im->colorspace = CS_RGB;
	}

	png_set_packing(arg->png_ptr);

	return 0;
}

int wi_png_load_image(struct image *im)
{
	int ret;
	struct wi_png_arg *arg = im->load_arg;

	if (setjmp(png_jmpbuf(arg->png_ptr)) ) {
		return -1;
	}

	ret = image_alloc_data(im);
	if (ret != 0) {
		png_error(arg->png_ptr, "alloc_image");
		return -1;
	}

	png_read_image(arg->png_ptr, im->row);

	return 0;
}

void wi_png_finish_read(struct image *im)
{
	struct wi_png_arg *arg;

	arg = im->load_arg;
	if (arg == NULL) return;

	if (setjmp(png_jmpbuf(arg->png_ptr)) ) {
		png_destroy_read_struct(&arg->png_ptr, &arg->info_ptr, NULL);
		FREE(im, arg);
		im->load_arg = NULL;
		return;
	}
	png_read_end(arg->png_ptr, NULL);
	png_destroy_read_struct(&arg->png_ptr, &arg->info_ptr, NULL);

	FREE(im, arg);
	im->load_arg = NULL;
	return;
}

struct loader png_loader = {
	.name			= "PNG",
	.ext			= ".png",
	.mime_type		= "image/png",

	.suitable		= wi_is_png,
	.init			= wi_png_init_read,
	.read_info		= wi_png_read_img_info,
	.load			= wi_png_load_image,
	.finish			= wi_png_finish_read,

	.save			= NULL,
};
