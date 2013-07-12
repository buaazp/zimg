#include "zcommon.h"

typedef struct zimg_req_s {
    char *md5;
    int width;
    int height;
    bool proportion;
    bool gray;
	char *rspPath;
} zimg_req_t;
