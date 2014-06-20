#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "webimg2.h"
#include "internal.h"

#include "../zlog.h"

#define default_malloc_args		NULL
#define default_realloc_args	NULL
#define default_free_args		NULL

uint8_t colorspace_comps[] = {1, 3, 4, 4};

static void * default_malloc(size_t size, void *arg)
{
	unused(arg);
	return malloc(size);
}

static void * default_realloc(void *ptr, size_t size, void *arg)
{
	unused(arg);
	return realloc(ptr, size);
}

static void default_free(void *ptr, void *arg)
{
	unused(arg);
	free(ptr);
}

int wi_set_format(struct image *im, char *fmt)
{
	struct loader *sv;

	sv = choose_saver(fmt);
	if (sv) {

		if (im->sv == sv) return WI_OK;

		im->sv = sv;
		im->format = sv->name;
		im->converted = 1;

		return WI_OK;
	}
	return WI_E_UNKNOW_FORMAT;
}

struct image * wi_new_image()
{
	int ret;
	struct image * im;

	im = (struct image *)calloc(1, sizeof(struct image));
	if (im) {
		ret = wi_init_image(im);
		if (ret != 0) {
			free(im);
			return NULL;
		}
	}

	return im;
}

void wi_free_image(struct image *im)
{
	if (im->magic == WEBIMG_MAGIC) {
		wi_destroy_image(im);
	}
	free(im);
}

int wi_init_image(struct image *im)
{
	im->magic			= WEBIMG_MAGIC;
	im->file			= 0;
	im->colorspace		= WI_DEFAULT_CS;
	im->quality			= 100;
	im->has_alpha		= 0;
	im->interlace		= 0;
	im->cols			= 0;
	im->rows			= 0;
	im->loaded			= 0;

	init_blob(&im->in_buf);
	init_blob(&im->ou_buf);

	/* default encoder "jpeg" must be implemented */
	wi_set_format(im, DEFAULT_FORMAT);

	im->ld				= NULL;
	im->load_arg		= NULL;

	im->data			= NULL;
	im->row				= NULL;

	im->converted		= 0;

	wi_set_malloc(im, default_malloc, default_malloc_args);
	wi_set_realloc(im, default_realloc, default_realloc_args);
	wi_set_free(im, default_free, default_free_args);

	return WI_OK;
}

void wi_destroy_image(struct image *im)
{
	assert(im->magic == WEBIMG_MAGIC);

	if (im->file) destroy_blob(im, &im->in_buf);
	destroy_blob(im, &im->ou_buf);

	if (im->ld) {
		im->ld->finish(im);
	}

	if (im->row) {
		FREE(im, im->row);
	}

	im->ld = NULL;

	return;
}

int image_alloc_data(struct image *im)
{
	int i;
	size_t rowlen, size;
	void *data = NULL;

	/* row index + image bitmap, do we need a delimiter? */
	rowlen = im->cols * get_components(im);
	size = (sizeof(uint8_t *) + rowlen * sizeof(uint8_t)) * im->rows;

	data = MALLOC(im, size);
	if (data == NULL) return -1;

	im->row = (uint8_t **)data;
	im->data = data + im->rows * sizeof(uint8_t *);

	for (i=0; i<im->rows; i++) {
		im->row[i] = im->data + i * rowlen;
	}

	return 0;
}

void image_free_data(struct image *im)
{
	if (im->row) {
		FREE(im, im->row);
	}
	im->row = NULL;
	im->data = NULL;

	return;
}

int wi_read_blob(struct image *im, void *blob, size_t len)
{
	assert(blob != NULL);
	assert(len > MIN_BLOB_LENGTH);

	int ret;
	struct loader *ld;

	ld = choose_loader(blob, len);
	if (ld == NULL) {
		return WI_E_UNKNOW_FORMAT;
	}

	im->in_buf.data = blob;
	im->in_buf.len = len;
	im->in_buf.ptr = im->in_buf.data + len;
	im->ld = ld;

	rewind_blob(&im->in_buf);

	if (ld->init != NULL) {
		ret = ld->init(im);
		if (ret != 0) {
			return WI_E_LOADER_INIT;
		}
	}

	ret = ld->read_info(im);
	if (ret != 0) {
		return WI_E_LOADER_LOAD;
	}

	im->format = ld->name;
	return WI_OK;
}

/* buf->data of new blob should be NULL */
int expand_blob(struct image *im, struct blob *buf, size_t len)
{
	void *ptr;
	size_t off;

	off = buf->ptr - buf->data;
	assert(off <= buf->len);

	ptr = REALLOC(im, buf->data, len);
	if (ptr == NULL) {
		return -1;
	}

	buf->data = ptr;
	buf->len = len;
	buf->ptr = ptr + off;
	return 0;
}

void free_blob(struct image *im, struct blob *buf)
{
	assert(buf->data != NULL);
	FREE(im, buf->data);
	init_blob(buf);
	return;
}

static int read_file(struct image *im, char *path, struct blob *buf)
{
	FILE *fp;
	int ret;
	struct stat st;

	init_blob(buf);

	fp = fopen(path, "rb");
	if (fp == NULL) return -1;

	ret = fstat(fileno(fp), &st);
	if (ret != 0) {
		fclose(fp);
		return -1;
	}
	if (st.st_size == 0) {
		fclose(fp);
		return -1;
	}

	ret = expand_blob(im, buf, st.st_size);
	if (ret != 0) {
		fclose(fp);
		return -1;
	}

	im->file = 1;

	ret = fread(buf->data, st.st_size, 1, fp);
	if (ret != 1) {
		free_blob(im, buf);
		fclose(fp);
		return -1;
	}
	buf->ptr = buf->data + st.st_size;
	fclose(fp);
	return 0;
}

static int write_file(const char *path, struct blob *buf)
{
	FILE *fp;
	int ret;

	fp = fopen(path, "w+b");
	if (fp == NULL) return -1;

	ret = fwrite(buf->data, buf->len, 1, fp);
	fclose(fp);

	return (ret == 1 ? 0 : -1);
}

int wi_read_file(struct image *im, char *path)
{
	int ret;
	
	ret = read_file(im, path, &im->in_buf);
	if (ret != 0) {
		return WI_E_READ_FILE;
	}

    //buaazp: read_file() has set im->in_buf, wi_read_blob() maybe reused.
	ret = wi_read_blob(im, im->in_buf.data, im->in_buf.len);
	return ret;
}

int wi_clone_image(struct image *src, struct image *dst)
{
	int ret;
	size_t rlen, size;

	memcpy(dst, src, sizeof(struct image));
	dst->file = 0;

	/* init loader */
	if (dst->ld != NULL && dst->ld->init != NULL) {
		ret = dst->ld->init(dst);
		if (ret != 0) {
			return WI_E_LOADER_INIT;
		}
	}

	ret = dst->ld->read_info(dst);
	if (ret != 0) {
		return WI_E_LOADER_INIT;
	}

	if (!dst->loaded) {
		return WI_OK;
	}

	/* copy img data */
	ret = image_alloc_data(dst);
	if (ret != 0) {
		return WI_E_LOADER_LOAD;
	}

	rlen = dst->cols * get_components(dst);
	size = (rlen * sizeof(uint8_t)) * dst->rows;
	memcpy(dst->data, src->data, size);

	return WI_OK;
}

static inline int wi_save(struct image *im)
{
	int ret;
	assert(im->sv != NULL);

	ret = load_image(im);
	if (ret != WI_OK) return WI_E_LOADER_LOAD;

	return im->sv->save(im);
}

uint8_t * wi_get_blob(struct image *im, size_t *size)
{
	int ret;

	ret = wi_save(im);
	if (ret == 0) {
		*size = im->ou_buf.len;
		return (uint8_t *)(im->ou_buf.data);
	}
	return NULL;
}

int wi_save_file(struct image *im, const char *path)
{
	int ret;

	ret = wi_save(im);
	if (ret != 0) {
		return WI_E_ENCODE;
	}

	ret = write_file(path, &im->ou_buf);
	return (ret == 0 ? WI_OK : WI_E_WRITE_FILE);
}

int wi_crop
(struct image *im, uint32_t x, uint32_t y, uint32_t cols, uint32_t rows)
{
	int i, ret;
	int line, xoff;
	uint8_t *dst, *src;
	uint8_t comps;

	if (im->cols < x + cols || im->rows < y + rows) return -1;
	if (im->cols == x + cols && im->rows == y + rows) return 0;

	ret = load_image(im);
	if (ret != WI_OK) return WI_E_LOADER_LOAD;

	comps = get_components(im);
	src = im->row[y] + x * comps;
	dst = im->row[0];
	line = cols * comps;
	xoff = im->cols * comps;

	for (i=0; i<rows; i++) {
		memcpy(dst, src, line);
		im->row[i] = dst;
		dst += line;
		src += xoff;
	}
	im->rows = rows;
	im->cols = cols;

	im->converted = 1;

	return 0;
}

void wi_set_quality(struct image *im, uint32_t quality)
{
	if (quality < 100) {
		if (im->quality != quality
				&& (strncmp(im->format, "JPEG", 4) == 0)) {
			im->converted = 1;
		}
		im->quality = quality;
	}
	return;
}

