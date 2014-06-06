#include "internal.h"

extern struct loader jpeg_loader;
extern struct loader webp_loader;
extern struct loader gif_loader;
extern struct loader png_loader;

struct loader * loaders[] = {
	&jpeg_loader,
	&webp_loader,
	&gif_loader,
	&png_loader,
	NULL,
};
