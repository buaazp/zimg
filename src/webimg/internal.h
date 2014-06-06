#ifndef __H_WI_INTERNAL_H__
#define __H_WI_INTERNAL_H__
#include <stdlib.h>

#include <assert.h>
#include "webimg2.h"

#define unused(x) ((void)(x))

#define WEBIMG_MAGIC			0xfee1900d

#define MIN_BLOB_LENGTH			16

#define DEFAULT_FORMAT			"JPEG"

struct libinfo {
	int				foo;
};

extern struct libinfo webimg_info;

struct loader {
	char *			name;
	char *			ext;
	char *			mime_type;

	int				(*suitable)(void *, size_t);
	int				(*init)(struct image *);
	int				(*read_info)(struct image *);
	int				(*resample)(struct image *, int, int);
	int				(*load)(struct image *);
	int				(*save)(struct image *);
	void			(*finish)(struct image *);
};

static inline void * MALLOC(struct image *im, size_t len)
{
	assert(im->malloc != NULL);
	return (im->malloc(len, im->alloc_args));
}

static inline void *REALLOC(struct image *im, void *ptr, size_t len)
{
	assert(im->realloc);
	return (im->realloc(ptr, len, im->reall_args));
}

static inline void FREE(struct image *im, void *ptr)
{
	assert(im->free);
	return (im->free(ptr, im->free_args));
}

static inline void init_blob(struct blob *buf)
{
	buf->data = buf->ptr = NULL;
	buf->len = 0;
}

static inline void
destroy_blob(struct image *im, struct blob *buf)
{
	if (buf->data != NULL) {
		FREE(im, buf->data);
	}
	init_blob(buf);
}

int expand_blob(struct image *im, struct blob *buf, size_t len);

void free_blob(struct image *im, struct blob *buf);

static inline void rewind_blob(struct blob *buf)
{
	buf->ptr = buf->data;
}

static inline size_t blob_left(struct blob *buf)
{
	return (buf->len - (buf->ptr - buf->data));
}

int image_alloc_data(struct image *im);
void image_free_data(struct image *im);

struct loader * choose_loader(void *blob, size_t len);
struct loader * choose_saver(const char *fmt);

static inline int load_image(struct image *im)
{
	int ret;

	if (!im->loaded) {
		ret = im->ld->load(im);
		if (ret != 0) return WI_E_LOADER_LOAD;
		im->loaded = 1;
	}

	return WI_OK;
}

extern uint8_t colorspace_comps[];
static inline uint8_t get_components(struct image *im)
{
	return colorspace_comps[im->colorspace];
}

static inline int presample
(struct image *im, uint32_t scale)
{
	if (!im->loaded && im->ld->resample != NULL) {
		return im->ld->resample(im, scale, 8);
	}
	return 0;
}

/* compiler */
#endif
