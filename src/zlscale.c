/*   
 *   zimg - high performance image storage and processing system.
 *       http://zimg.buaa.us 
 *   
 *   Copyright (c) 2013-2014, Peter Zhao <zp@buaa.us>.
 *   All rights reserved.
 *   
 *   Use and distribution licensed under the BSD license.
 *   See the LICENSE file for full text.
 * 
 */

/**
 * @file zlscale.c
 * @brief processing image with lua script.
 * @author 招牌疯子 zp@buaa.us
 * @version 3.0.0
 * @date 2014-08-14
 */

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <wand/magick_wand.h>
#include "zcommon.h"
#include "zlog.h"
#include "zlscale.h"

int lua_convert(MagickWand *im, zimg_req_t *req);

static int get_wi_cols(lua_State *L) {
    lua_arg *larg = pthread_getspecific(thread_key);
    unsigned long cols = MagickGetImageWidth(larg->img);
    lua_pushnumber(L, cols);
    return 1;
}

static int get_wi_rows(lua_State *L) {
    lua_arg *larg = pthread_getspecific(thread_key);
    unsigned long rows = MagickGetImageHeight(larg->img);
    lua_pushnumber(L, rows);
    return 1;
}

static int get_wi_quality(lua_State *L) {
    lua_arg *larg = pthread_getspecific(thread_key);
    int quality = MagickGetImageCompressionQuality(larg->img);
    quality = (quality == 0 ? 100 : quality);
    lua_pushnumber(L, quality);
    return 1;
}

static int get_wi_format(lua_State *L) {
    lua_arg *larg = pthread_getspecific(thread_key);
    char *format = MagickGetImageFormat(larg->img);
    LOG_PRINT(LOG_DEBUG, "get_wi_format: %s", format);
    lua_pushstring(L, format);
    free(format);
    return 1;
}

static int scale_wi (lua_State *L) {
    double cols = lua_tonumber(L, 1);
    double rows = lua_tonumber(L, 2);

    LOG_PRINT(LOG_DEBUG, "cols = %f rows = %f", cols, rows);
    lua_arg *larg = pthread_getspecific(thread_key);
    int ret = MagickResizeImage(larg->img, cols, rows, LanczosFilter, 1.0);
    //int ret = MagickScaleImage(larg->img, cols, rows);
    lua_pushnumber(L, ret);
    return 1;
}

static int crop_wi (lua_State *L) {
    double    x = lua_tonumber(L, 1);
    double    y = lua_tonumber(L, 2);
    double cols = lua_tonumber(L, 3);
    double rows = lua_tonumber(L, 4);
    
    lua_arg *larg = pthread_getspecific(thread_key);
    int ret = MagickCropImage(larg->img, cols, rows, x, y);
    lua_pushnumber(L, ret);
    return 1;
}

static int rotate_wi (lua_State *L) {
    int ret = -1;
    double rotate = lua_tonumber(L, 1);

    lua_arg *larg = pthread_getspecific(thread_key);
    LOG_PRINT(LOG_DEBUG, "wi_rotate(im, %d)", rotate);
    PixelWand *background = NewPixelWand();
    if (background == NULL) {
        lua_pushnumber(L, ret);
        return 1;
    }
    ret = PixelSetColor(background, "white");
    if (ret != MagickTrue) {
        DestroyPixelWand(background);
        lua_pushnumber(L, ret);
        return 1;
    }
    ret = MagickRotateImage(larg->img, background, rotate);
    LOG_PRINT(LOG_DEBUG, "rotate() ret = %d", ret);

    DestroyPixelWand(background);
    lua_pushnumber(L, ret);
    return 1;
}

static int gray_wi (lua_State *L) {
    lua_arg *larg = pthread_getspecific(thread_key);
    int ret = MagickSetImageType(larg->img, GrayscaleType);
    LOG_PRINT(LOG_INFO, "gray_wi: ret = %d", ret);
    lua_pushnumber(L, ret);
    return 1;
}

static int set_wi_quality (lua_State *L) {
    int quality = lua_tonumber(L, 1);
    
    lua_arg *larg = pthread_getspecific(thread_key);
    int ret = MagickSetImageCompressionQuality(larg->img, quality);
    lua_pushnumber(L, ret);
    return 1;
}

static int set_wi_format (lua_State *L) {
    const char *format = lua_tostring(L, 1);
    
    lua_arg *larg = pthread_getspecific(thread_key);
    int ret = MagickSetImageFormat(larg->img, format);
    LOG_PRINT(LOG_DEBUG, "set_wi_format: %s ret = %d", format, ret);
    lua_pushnumber(L, ret);
    return 1;
}

static int zimg_type(lua_State *L)
{
    lua_arg *larg = pthread_getspecific(thread_key);
    lua_pushstring(L, larg->trans_type);
    LOG_PRINT(LOG_DEBUG, "zimg_type: %s", larg->trans_type);
    return 1;
}

static int zimg_ret(lua_State *L)
{
    lua_arg *larg = pthread_getspecific(thread_key);
    larg->lua_ret = lua_tonumber(L, 1);
    return 0;
}

const struct luaL_reg zimg_lib[] = {
    //{"__gc",                destroy_wi_image    },
    {"cols",                get_wi_cols         },
    {"rows",                get_wi_rows         },
    {"quality",             get_wi_quality      },
    {"format",              get_wi_format       },
    {"scale",               scale_wi            },
    {"crop",                crop_wi             },
    {"rotate",              rotate_wi           },
    {"gray",                gray_wi             },
    {"set_quality",         set_wi_quality      },
    {"set_format",          set_wi_format       },
    {"type",                zimg_type           },
    {"ret",                 zimg_ret            },
    {NULL,                  NULL                }
};

static int lua_log_print(lua_State *L)
{
    int log_level = lua_tonumber(L, 1);
    const char *log_str = lua_tostring(L, 2);
    LOG_PRINT(log_level, "zimg_lua: %s", log_str);
    return 0;
}

const struct luaL_Reg loglib[] = {
    {"print",       lua_log_print   },
    {NULL,          NULL            }
};

/**
 * @brief lua_convert the function of convert image by lua
 *
 * @param im the im object which will be convert
 * @param req the zimg request
 *
 * @return 1 for OK and -1 for fail
 */
int lua_convert(MagickWand *im, zimg_req_t *req)
{
    int ret = -1;
    LOG_PRINT(LOG_DEBUG, "lua_convert: %s", req->type);
    MagickResetIterator(im);

    if(req->thr_arg->L != NULL)
    {
        lua_arg *larg = (lua_arg *)malloc(sizeof(lua_arg));
        if(larg == NULL)
            return -1;
        larg->lua_ret = ret;
        larg->trans_type = req->type;
        larg->img = im;
        pthread_setspecific(thread_key, larg);
        //luaL_dofile(req->thr_arg->L, settings.script_name);
        lua_getglobal(req->thr_arg->L, "f");
        if(lua_pcall(req->thr_arg->L, 0, 0, 0) != 0)
        {
            LOG_PRINT(LOG_WARNING, "lua f() failed!");
        }

        ret = larg->lua_ret;
        free(larg);
    }
    else
        LOG_PRINT(LOG_WARNING, "no lua_stats, lua_convert failed!");

    return ret;
}

