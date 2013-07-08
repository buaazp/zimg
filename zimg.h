#include "zcommon.h"

typedef struct zimg_req_s {
    char *md5;
    int width;
    int height;
    bool proportion;
    bool gray;
} zimg_req_t;
