#include "zcommon.h"

static void kmp_init(const unsigned char *pattern, int pattern_size);
int kmp(const unsigned char *matcher, int mlen, const unsigned char *pattern, int plen);
int getType(const char *filename, char *type);
int isImg(const char *filename);
int isDir(const char *path);
int mkDir(const char *path);
