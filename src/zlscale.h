#ifndef ZLSCALE_H
#define ZLSCALE_H

#include "webimg/webimg2.h"

typedef struct lua_arg_s {
    struct image *img;
    char *trans_type;
    int lua_ret;
} lua_arg;

int lua_convert(struct image *im, const char *type);

#endif
