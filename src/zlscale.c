#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

#include "zcommon.h"
#include "zlog.h"
#include "zlscale.h"
#include "webimg/webimg2.h"

int lua_convert(struct image *im, zimg_req_t *req);

static int get_wi_cols(lua_State *L) {
    lua_arg *larg = pthread_getspecific(thread_key);
    int cols = larg->img->cols;
    lua_pushnumber(L, cols);
    return 1;
}

static int get_wi_rows(lua_State *L) {
    lua_arg *larg = pthread_getspecific(thread_key);
    int rows = larg->img->rows;
    lua_pushnumber(L, rows);
    return 1;
}

static int get_wi_quality(lua_State *L) {
    lua_arg *larg = pthread_getspecific(thread_key);
    int quality = larg->img->quality;
    lua_pushnumber(L, quality);
    return 1;
}

static int get_wi_format(lua_State *L) {
    lua_arg *larg = pthread_getspecific(thread_key);
    char *format = larg->img->format;
    LOG_PRINT(LOG_INFO, "get_wi_format: %s", format);
    lua_pushstring(L, format);
    return 1;
}

static int scale_wi (lua_State *L) {
    double cols = lua_tonumber(L, 1);
    double rows = lua_tonumber(L, 2);

    LOG_PRINT(LOG_DEBUG, "cols = %f rows = %f", cols, rows);
    lua_arg *larg = pthread_getspecific(thread_key);
    int ret = wi_scale(larg->img, cols, rows);
    LOG_PRINT(LOG_DEBUG, "ret = %d", ret);
    if (larg->img->colorspace == CS_RGB) {
        LOG_PRINT(LOG_DEBUG, "rgb!");
    } else if (larg->img->colorspace == CS_GRAYSCALE) {
        LOG_PRINT(LOG_DEBUG, "gray!");
    } else {
        LOG_PRINT(LOG_DEBUG, "other!");
    }
    lua_pushnumber(L, ret);
    return 1;
}

static int crop_wi (lua_State *L) {
    double    x = lua_tonumber(L, 1);
    double    y = lua_tonumber(L, 2);
    double cols = lua_tonumber(L, 3);
    double rows = lua_tonumber(L, 4);
    
    lua_arg *larg = pthread_getspecific(thread_key);
    int ret = wi_crop(larg->img, x, y, cols, rows);
    lua_pushnumber(L, ret);
    return 1;
}

static int gray_wi (lua_State *L) {
    lua_arg *larg = pthread_getspecific(thread_key);
    int ret = wi_gray(larg->img);
    if (larg->img->colorspace == CS_RGB) {
        LOG_PRINT(LOG_DEBUG, "rgb!");
    } else if (larg->img->colorspace == CS_GRAYSCALE) {
        LOG_PRINT(LOG_DEBUG, "gray!");
    } else {
        LOG_PRINT(LOG_DEBUG, "other!");
    }
    lua_pushnumber(L, ret);
    return 1;
}

static int set_wi_quality (lua_State *L) {
    int quality = lua_tonumber(L, 1);
    
    lua_arg *larg = pthread_getspecific(thread_key);
    wi_set_quality(larg->img, quality);
    return 0;
}

static int set_wi_format (lua_State *L) {
    const char *format = lua_tostring(L, 1);
    
    lua_arg *larg = pthread_getspecific(thread_key);
    int ret = wi_set_format(larg->img, (char *)format);
    LOG_PRINT(LOG_INFO, "set_wi_format: %s ret = %d", format, ret);
    lua_pushnumber(L, ret);
    return 1;
}

const struct luaL_reg webimg_lib[] = {
    //{"__gc",                destroy_wi_image    },
    {"cols",                get_wi_cols         },
    {"rows",                get_wi_rows         },
    {"quality",             get_wi_quality      },
    {"format",              get_wi_format       },
    {"scale",               scale_wi            },
    {"crop",                crop_wi             },
    {"gray",                gray_wi             },
    {"wi_get_cols",         get_wi_cols         },
    {"wi_get_rows",         get_wi_rows         },
    {"wi_get_quality",      get_wi_quality      },
    {"wi_set_quality",      set_wi_quality      },
    {"wi_get_format",       get_wi_format       },
    {"wi_set_format",       set_wi_format       },
    {"wi_scale",            scale_wi            },
    {"wi_crop",             crop_wi             },
    {"wi_gray",             gray_wi             },
    {NULL,                  NULL                }
};

static int req_pull(lua_State *L)
{
    lua_arg *larg = pthread_getspecific(thread_key);
    lua_pushstring(L, larg->trans_type);
    LOG_PRINT(LOG_INFO, "req_pull: %s", larg->trans_type);
    return 1;
}

static int req_push(lua_State *L)
{
    lua_arg *larg = pthread_getspecific(thread_key);
    larg->lua_ret = lua_tonumber(L, 1);
    return 0;
}

const struct luaL_Reg requestlib[] = {
    {"pull",        req_pull    },
    {"push",        req_push    },
    {NULL,          NULL        }
};

int lua_convert(struct image *im, zimg_req_t *req)
{
    int ret = -1;
    LOG_PRINT(LOG_DEBUG, "lua_convert: %s", req->type);

    if(req->thr_arg->L != NULL)
    {
        lua_arg *larg = (lua_arg *)malloc(sizeof(lua_arg));
        if(larg == NULL)
            return -1;
        larg->lua_ret = ret;
        larg->trans_type = req->type;
        larg->img = im;
        pthread_setspecific(thread_key, larg);
        luaL_dofile(req->thr_arg->L, settings.script_name);

        ret = larg->lua_ret;
        free(larg);
    }
    else
        LOG_PRINT(LOG_WARNING, "no lua_stats, lua_convert failed!");

    return ret;
}

