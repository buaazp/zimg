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
 * @file zssdb.c
 * @brief Get and save image for ssdb/redis backend functions.
 * @author 招牌疯子 zp@buaa.us
 * @version 2.0.0
 * @date 2014-04-16
 */

#include <wand/MagickWand.h>
#include "zssdb.h"
#include "zlog.h"
#include "zcache.h"
//#include <hiredis/hiredis.h>
#include "hiredis.h"
#include "zutil.h"

extern struct setting settings;

int get_img_mode_ssdb(zimg_req_t *req, char **buff_ptr, size_t *img_size);
int save_img_ssdb(const char *cache_key, const char *buff, const size_t len);
int get_img_ssdb(const char *cache_key, char **buff, size_t *len);


/**
 * @brief get_img_mode_ssdb Get image from ssdb backend.
 *
 * @param req zimg request struct
 * @param buff_ptr buff pointer
 * @param img_size buff size
 *
 * @return 1 for success and -1 for failed
 */
int get_img_mode_ssdb(zimg_req_t *req, char **buff_ptr, size_t *img_size)
{
    int result = -1;
    char *cache_key = (char *)malloc(strlen(req->md5) + 32);
    char *img_format = NULL;
    size_t len;

    MagickBooleanType status;
    MagickWand *magick_wand = NULL;

    bool got_color = false;

    LOG_PRINT(LOG_DEBUG, "get_img() start processing zimg request...");

    //sprintf(cache_key, "img:%s:%d:%d:%d:%d", req->md5, req->width, req->height, req->proportion, req->gray);
    gen_key(cache_key, req->md5, 4, req->width, req->height, req->proportion, req->gray);
    if(find_cache_bin(cache_key, buff_ptr, img_size) == 1)
    {
        LOG_PRINT(LOG_DEBUG, "Hit Cache[Key: %s].", cache_key);
        result = 1;
        goto done;
    }
    LOG_PRINT(LOG_DEBUG, "Start to Find the Image...");
    if(get_img_ssdb(cache_key, buff_ptr, img_size) == 1)
    {
        LOG_PRINT(LOG_DEBUG, "Get image [%s] from ssdb succ.", cache_key);
        if(*img_size < CACHE_MAX_SIZE)
        {
            set_cache_bin(cache_key, *buff_ptr, *img_size);
        }
        result = 1;
        goto done;
    }

    magick_wand = NewMagickWand();
    //sprintf(cache_key, "img:%s:%d:%d:%d:0", req->md5, req->width, req->height, req->proportion);
    gen_key(cache_key, req->md5, 3, req->width, req->height, req->proportion);
    if(find_cache_bin(cache_key, buff_ptr, img_size) == 1)
    {
        LOG_PRINT(LOG_DEBUG, "Hit Color Image Cache[Key: %s, len: %d].", cache_key, *img_size);
        status = MagickReadImageBlob(magick_wand, *buff_ptr, *img_size);
        if(status == MagickFalse)
        {
            LOG_PRINT(LOG_WARNING, "Color Image Cache[Key: %s] is Bad. Remove.", cache_key);
            del_cache(cache_key);
        }
        else
        {
            got_color = true;
            LOG_PRINT(LOG_DEBUG, "Read Image from Color Image Cache[Key: %s, len: %d] Succ. Goto Convert.", cache_key, *img_size);
            goto convert;
        }
    }
    if(get_img_ssdb(cache_key, buff_ptr, img_size) == 1)
    {
        LOG_PRINT(LOG_DEBUG, "Get color image [%s] from ssdb.", cache_key);
        status = MagickReadImageBlob(magick_wand, *buff_ptr, *img_size);
        if(status == MagickTrue)
        {
            got_color = true;
            LOG_PRINT(LOG_DEBUG, "Read Image from Color Image[%s] Succ. Goto Convert.", cache_key);
            *buff_ptr = (char *)MagickGetImageBlob(magick_wand, img_size);
            if(*img_size < CACHE_MAX_SIZE)
            {
                set_cache_bin(cache_key, *buff_ptr, *img_size);
            }
            goto convert;
        }
    }

    //sprintf(cache_key, "img:%s:0:0:1:0", req->md5);
    gen_key(cache_key, req->md5, 0);
    if(find_cache_bin(cache_key, buff_ptr, img_size) == 1)
    {
        LOG_PRINT(LOG_DEBUG, "Hit Cache[Key: %s].", cache_key);
    }
    else
    {
        if(get_img_ssdb(cache_key, buff_ptr, img_size) == -1)
        {
            LOG_PRINT(LOG_ERROR, "Get image [%s] from ssdb failed.", cache_key);
            goto done;
        }
        else if(*img_size < CACHE_MAX_SIZE)
        {
            set_cache_bin(cache_key, *buff_ptr, *img_size);
        }
    }

    status = MagickReadImageBlob(magick_wand, *buff_ptr, *img_size);
    if(status == MagickFalse)
    {
        ThrowWandException(magick_wand);
        del_cache(cache_key);
        LOG_PRINT(LOG_ERROR, "Get image [%s] from ssdb failed.", cache_key);
        goto done;
    }

    if(!(req->width == 0 && req->height == 0))
    {
        int width, height;
        width = req->width;
        height = req->height;
        float owidth = MagickGetImageWidth(magick_wand);
        float oheight = MagickGetImageHeight(magick_wand);
        if(width <= owidth && height <= oheight)
        {
            if(req->proportion == 1)
            {
                if(req->width != 0 && req->height == 0)
                {
                    height = width * oheight / owidth;
                }
                else
                {
                    width = height * owidth / oheight;
                }
            }
            status = MagickResizeImage(magick_wand, width, height, LanczosFilter, 1.0);
            if(status == MagickFalse)
            {
                LOG_PRINT(LOG_ERROR, "Image[%s] Resize Failed!", cache_key);
                goto done;
            }

            LOG_PRINT(LOG_DEBUG, "Resize img succ.");
        }
        else
        {
            LOG_PRINT(LOG_DEBUG, "Args width/height is bigger than real size, return original image.");
            result = 1;
            goto done;
        }
    }


convert:

    //compress image
    if(got_color == false)
    {
        LOG_PRINT(LOG_INFO, "Start to Compress the Image!");
        img_format = MagickGetImageFormat(magick_wand);
        LOG_PRINT(LOG_INFO, "Image Format is %s", img_format);
        if(strcmp(img_format, "JPEG") != 0)
        {
            LOG_PRINT(LOG_INFO, "Convert Image Format from %s to JPEG.", img_format);
            status = MagickSetImageFormat(magick_wand, "JPEG");
            if(status == MagickFalse)
            {
                LOG_PRINT(LOG_WARNING, "Image[%s] Convert Format Failed!", cache_key);
            }
            LOG_PRINT(LOG_INFO, "Compress Image with JPEGCompression");
            status = MagickSetImageCompression(magick_wand, JPEGCompression);
            if(status == MagickFalse)
            {
                LOG_PRINT(LOG_WARNING, "Image[%s] Compression Failed!", cache_key);
            }
        }
        size_t quality = MagickGetImageCompressionQuality(magick_wand) * 0.75;
        LOG_PRINT(LOG_INFO, "Image Compression Quality is %u.", quality);
        if(quality == 0)
        {
            quality = 75;
        }
        LOG_PRINT(LOG_INFO, "Set Compression Quality to 75%.");
        status = MagickSetImageCompressionQuality(magick_wand, quality);
        if(status == MagickFalse)
        {
            LOG_PRINT(LOG_WARNING, "Set Compression Quality Failed!");
        }

        //strip image EXIF infomation
        LOG_PRINT(LOG_INFO, "Start to Remove Exif Infomation of the Image...");
        status = MagickStripImage(magick_wand);
        if(status == MagickFalse)
        {
            LOG_PRINT(LOG_WARNING, "Remove Exif Infomation of the ImageFailed!");
        }

        *buff_ptr = (char *)MagickGetImageBlob(magick_wand, img_size);
        if(*buff_ptr == NULL)
        {
            LOG_PRINT(LOG_ERROR, "Magick Get Image Blob Failed!");
            goto done;
        }
        gen_key(cache_key, req->md5, 3, req->width, req->height, req->proportion);
        save_img_ssdb(cache_key, *buff_ptr, *img_size);
        if(*img_size < CACHE_MAX_SIZE)
        {
            set_cache_bin(cache_key, *buff_ptr, *img_size);
        }
    } 

    //gray image
    if(req->gray == 1)
    {
        LOG_PRINT(LOG_INFO, "Start to Remove Color!");
        status = MagickSetImageColorspace(magick_wand, GRAYColorspace);
        if(status == MagickFalse)
        {
            LOG_PRINT(LOG_ERROR, "Image[%s] Remove Color Failed!", cache_key);
            goto done;
        }
        LOG_PRINT(LOG_DEBUG, "Image Remove Color Finish!");


        *buff_ptr = (char *)MagickGetImageBlob(magick_wand, img_size);
        if(*buff_ptr == NULL)
        {
            LOG_PRINT(LOG_ERROR, "Magick Get Image Blob Failed!");
            goto done;
        }
    }
    //sprintf(cache_key, "img:%s:%d:%d:%d:%d", req->md5, req->width, req->height, req->proportion, req->gray);
    gen_key(cache_key, req->md5, 4, req->width, req->height, req->proportion, req->gray);
    save_img_ssdb(cache_key, *buff_ptr, *img_size);
    if(*img_size < CACHE_MAX_SIZE)
    {
        set_cache_bin(cache_key, *buff_ptr, *img_size);
    }
    result = 1;

done:
    if(magick_wand)
    {
        magick_wand=DestroyMagickWand(magick_wand);
    }
    if(img_format)
        free(img_format);
    if(cache_key)
        free(cache_key);
    return result;
}

/**
 * @brief save_img_ssdb Save image to ssdb backends.
 *
 * @param cache_key image key
 * @param buff image buff
 * @param len image buff length
 *
 * @return 1 for success and -1 for fail
 */
int save_img_ssdb(const char *cache_key, const char *buff, const size_t len)
{
    redisContext* c = redisConnect(settings.ssdb_ip, settings.ssdb_port);
    if(c->err)
    {
        redisFree(c);
        LOG_PRINT(LOG_ERROR, "Connect to ssdb server faile");
        return -1;
    }
    LOG_PRINT(LOG_INFO, "Connect to ssdb server Success");

    redisReply *r = (redisReply*)redisCommand(c, "SET %s %b", cache_key, buff, len);
    if( NULL == r)
    {
        LOG_PRINT(LOG_ERROR, "Execut ssdb command failure");
        redisFree(c);
        return -1;
    }
    if( !(r->type == REDIS_REPLY_STATUS && strcasecmp(r->str,"OK")==0))
    {
        LOG_PRINT(LOG_ERROR, "Failed to execute save [%s] to ssdb.", cache_key);
        freeReplyObject(r);
        redisFree(c);
        return -1;
    }
    freeReplyObject(r);
    redisFree(c);
    LOG_PRINT(LOG_INFO, "Succeed to save [%s] to ssdb. length = [%d].", cache_key, len);

    return 1;
}

/**
 * @brief get_img_ssdb Get image from ssdb backend by key
 *
 * @param cache_key image key
 * @param buff buffer to storage image
 * @param len image size
 *
 * @return 1 for success and -1 for fail
 */
int get_img_ssdb(const char *cache_key, char **buff, size_t *len)
{
    redisContext* c = redisConnect(settings.ssdb_ip, settings.ssdb_port);
    if(c->err)
    {
        redisFree(c);
        LOG_PRINT(LOG_ERROR, "Connect to ssdb server faile");
        return -1;
    }
    LOG_PRINT(LOG_INFO, "Connect to ssdb server Success");

    redisReply *r = (redisReply*)redisCommand(c, "GET %s", cache_key);
    if( NULL == r)
    {
        LOG_PRINT(LOG_ERROR, "Execut ssdb command failure");
        redisFree(c);
        return -1;
    }
    if( r->type != REDIS_REPLY_STRING )
    {
        LOG_PRINT(LOG_ERROR, "Failed to execute get [%s] from ssdb.", cache_key);
        freeReplyObject(r);
        redisFree(c);
        return -1;
    }

    *len = r->len;
    *buff = (char *)malloc(r->len);
    memcpy(*buff, r->str, r->len);

    freeReplyObject(r);
    redisFree(c);
    LOG_PRINT(LOG_INFO, "Succeed to get [%s] from ssdb. length = [%d].", cache_key, *len);

    return 1;
}

