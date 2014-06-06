#ifndef __H_WEBIMG_H__
#define __H_WEBIMG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdint.h>

#define WI_LIB_VERSION		"0.0.1a"

#define WI_DEFAULT_CS		CS_RGB

#define WI_DEFAULT_QUALITY	85
#define WI_MAX_WIDTH		65535
#define WI_MAX_HEIGHT		65535

struct blob {
	void *					data;
	void *					ptr;
	size_t					len;
};

typedef enum {
	CS_GRAYSCALE			= 0,
	CS_RGB,
	CS_RGBA,
	CS_CMYK,
} cs_t;

enum wi_errno {
	WI_OK					= 0,
	WI_E_UNKNOW_FORMAT,
	WI_E_LOADER_INIT,
	WI_E_LOADER_LOAD,
	WI_E_READ_FILE,
	WI_E_WRITE_FILE,
	WI_E_ENCODE,
};

struct image {
	int						magic;
	int						file;

	char *					format;
	cs_t					colorspace;

	uint32_t				quality;
	int						has_alpha;
	int						interlace;

	uint32_t				cols;
	uint32_t				rows;

	int						loaded;
	struct blob				in_buf;
	struct blob				ou_buf;

	void *					load_arg;
	struct loader *			ld;
	struct loader *			sv;

	uint8_t *				data;
	uint8_t **				row;

	/* mem */
	void *					alloc_args;
	void *					reall_args;
	void *					free_args;
	void *					(*malloc)(size_t, void *);
	void *					(*realloc)(void *, size_t, void *);
	void					(*free)(void *, void *);

	void *					memtrack;
	int						converted;
};

static inline void wi_set_malloc
(struct image *im, void * (*m)(size_t, void *), void *arg)
{
	im->malloc = m;
	im->alloc_args = arg;
}
static inline void wi_set_realloc
(struct image *im, void *(*r)(void *, size_t, void *), void *arg)
{
	im->realloc = r;
	im->reall_args = arg;
}

static inline void wi_set_free
(struct image *im, void (*f)(void*, void*), void *arg)
{
	im->free = f;
	im->free_args = arg;
}

int wi_set_format(struct image *im, char *fmt);
void wi_set_quality(struct image *im, uint32_t quality);

static inline int wi_is_valid_size(uint32_t width, uint32_t height)
{
	return (width <= WI_MAX_WIDTH && height <= WI_MAX_HEIGHT
			&& width > 0 && height > 0);
}

struct image * wi_new_image();
void wi_free_image(struct image *);
int wi_init_image(struct image *);
void wi_destroy_image(struct image *);
int wi_clone_image(struct image *src, struct image *dst);

int wi_read_file(struct image *im, char *path);
int wi_read_blob(struct image *im, void *blob, size_t len);
int wi_save_file(struct image *im, const char *path);
uint8_t * wi_get_blob(struct image *im, size_t *size);

int wi_scale(struct image *im, uint32_t cols, uint32_t rows);

int wi_crop(struct image *, uint32_t, uint32_t, uint32_t, uint32_t);


#ifdef __cplusplus
}
#endif

#endif
