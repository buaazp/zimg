#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

#include "zcommon.h"
#include "zlog.h"
#include "zlscale.h"
#include "webimg/webimg2.h"

int lua_convert(struct image *im, const char *type);

//const static char *lua_path = "process.lua";
const static char *lua_path = "wk.lua";
static struct image *img;
static char *trans_type;
static int lua_ret;

static int get_wi_cols(lua_State *L) {
    int cols = img->cols;
    lua_pushnumber(L, cols);
    return 1;
}

static int get_wi_rows(lua_State *L) {
    int rows = img->rows;
    lua_pushnumber(L, rows);
    return 1;
}

static int get_wi_quality(lua_State *L) {
    int quality = img->quality;
    lua_pushnumber(L, quality);
    return 1;
}

static int get_wi_format(lua_State *L) {
    char *format = img->format;
    LOG_PRINT(LOG_INFO, "get_wi_format: %s", format);
    lua_pushstring(L, format);
    return 1;
}

static int scale_wi (lua_State *L) {
    double cols = lua_tonumber(L, 1);
    double rows = lua_tonumber(L, 2);

    LOG_PRINT(LOG_DEBUG, "cols = %f rows = %f", cols, rows);
    int ret = wi_scale(img, cols, rows);
    LOG_PRINT(LOG_DEBUG, "ret = %d", ret);
    if (img->colorspace == CS_RGB) {
        LOG_PRINT(LOG_DEBUG, "rgb!");
    } else if (img->colorspace == CS_GRAYSCALE) {
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
    
    int ret = wi_crop(img, x, y, cols, rows);
    lua_pushnumber(L, ret);
    return 1;
}

static int gray_wi (lua_State *L) {
    int ret = wi_gray(img);
    if (img->colorspace == CS_RGB) {
        LOG_PRINT(LOG_DEBUG, "rgb!");
    } else if (img->colorspace == CS_GRAYSCALE) {
        LOG_PRINT(LOG_DEBUG, "gray!");
    } else {
        LOG_PRINT(LOG_DEBUG, "other!");
    }
    lua_pushnumber(L, ret);
    return 1;
}

static int set_wi_quality (lua_State *L) {
    int quality = lua_tonumber(L, 1);
    
    wi_set_quality(img, quality);
    return 0;
}

static int set_wi_format (lua_State *L) {
    const char *format = lua_tostring(L, 1);
    
    int ret = wi_set_format(img, (char *)format);
    LOG_PRINT(LOG_INFO, "set_wi_format: %s ret = %d", format, ret);
    lua_pushnumber(L, ret);
    return 1;
}

static const struct luaL_reg webimg_lib[] = {
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
    lua_pushstring(L, trans_type);
    LOG_PRINT(LOG_INFO, "req_pull: %s", trans_type);
    return 1;
}

static int req_push(lua_State *L)
{
    lua_ret = lua_tonumber(L, 1);
    return 0;
}

static const struct luaL_Reg requestlib [] = {
    {"pull",        req_pull    },
    {"push",        req_push    },
    {NULL,          NULL        }
};

int lua_convert(struct image *im, const char *type)
{
    LOG_PRINT(LOG_INFO, "lua_convert: %s", type);
    lua_State* L = luaL_newstate(); 
    luaL_openlibs(L);
    luaL_openlib(L, "request", requestlib, 0);
    luaL_openlib(L, "webimg", webimg_lib, 0);
    lua_ret = -1;
    trans_type = (char *)type;
    img = im;
    luaL_dofile(L, lua_path);
    lua_close(L);

    return lua_ret;
}

