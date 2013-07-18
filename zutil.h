#ifndef ZUTIL_H
#define ZUTIL_H
#endif

#include "zcommon.h"

static void kmp_init(const unsigned char *pattern, int pattern_size);
int kmp(const unsigned char *matcher, int mlen, const unsigned char *pattern, int plen);
int get_type(const char *filename, char *type);
int is_img(const char *filename);
int is_dir(const char *path);
int mk_dir(const char *path);
