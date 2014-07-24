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
 * @file zdb.c
 * @brief Get and save image for ssdb/redis and beansdb/memcachedb backend functions.
 * @author 招牌疯子 zp@buaa.us
 * @version 2.0.0
 * @date 2014-04-21
 */

#include <string.h>
#include <wand/MagickWand.h>
#include <hiredis/hiredis.h>
#include "webimg/webimg2.h"
#include "zdb.h"
#include "zlog.h"
#include "zcache.h"
#include "zutil.h"
#include "zscale.h"
#include "zlscale.h"

int get_img_mode_db(zimg_req_t *req, evhtp_request_t *request);
int get_img_mode_db2(zimg_req_t *req, evhtp_request_t *request);

int get_img_db(thr_arg_t *thr_arg, const char *cache_key, char **buff, size_t *len);
int get_img_beansdb(memcached_st *memc, const char *key, char **value_ptr, size_t *len);
int get_img_ssdb(redisContext* c, const char *cache_key, char **buff, size_t *len);

int save_img_db(thr_arg_t *thr_arg, const char *cache_key, const char *buff, const size_t len);
int save_img_beansdb(memcached_st *memc, const char *key, const char *value, const size_t len);
int save_img_ssdb(redisContext* c, const char *cache_key, const char *buff, const size_t len);

int admin_img_mode_db(thr_arg_t *thr_arg, char *md5, int t);

int exist_db(thr_arg_t *thr_arg, const char *cache_key);
int exist_beansdb(memcached_st *memc, const char *key);
int exist_ssdb(redisContext* c, const char *cache_key);

int del_db(thr_arg_t *thr_arg, const char *cache_key);
int del_beansdb(memcached_st *memc, const char *key);
int del_ssdb(redisContext* c, const char *cache_key);

int find_beansdb(memcached_st *memc, const char *key, char *value);
int set_beansdb(memcached_st *memc, const char *key, const char *value);

/**
 * @brief get_img_mode_db Get image from db mode backend.
 *
 * @param req zimg request struct
 * @param buff_ptr buff pointer
 * @param img_size buff size
 *
 * @return 1 for success and -1 for failed
 */
int get_img_mode_db(zimg_req_t *req, evhtp_request_t *request)
{
    int result = -1;
    char cache_key[CACHE_KEY_SIZE];
    char *img_format = NULL;
    char *buff_ptr;
    size_t img_size;

    MagickBooleanType status;
    MagickWand *magick_wand = NULL;

    bool got_rsp = true;
    bool got_color = false;

    LOG_PRINT(LOG_DEBUG, "get_img() start processing zimg request...");

    if(req->gray == 1)
    {
        gen_key(cache_key, req->md5, 4, req->width, req->height, req->proportion, req->gray);
    }
    else
    {
        if(req->proportion == 0 && req->width == 0 && req->height == 0)
        {
            gen_key(cache_key, req->md5, 0);
        }
        else
        {
            gen_key(cache_key, req->md5, 3, req->width, req->height, req->proportion);
        }
    }
    if(find_cache_bin(req->thr_arg, cache_key, &buff_ptr, &img_size) == 1)
    {
        LOG_PRINT(LOG_DEBUG, "Hit Cache[Key: %s].", cache_key);
        goto done;
    }
    LOG_PRINT(LOG_DEBUG, "Start to Find the Image...");
    if(get_img_db(req->thr_arg, cache_key, &buff_ptr, &img_size) == 1)
    {
        LOG_PRINT(LOG_DEBUG, "Get image [%s] from backend db succ.", cache_key);
        if(img_size < CACHE_MAX_SIZE)
        {
            set_cache_bin(req->thr_arg, cache_key, buff_ptr, img_size);
        }
        goto done;
    }

    magick_wand = NewMagickWand();
    got_rsp = false;
    if(req->gray == 1)
    {
        gen_key(cache_key, req->md5, 3, req->width, req->height, req->proportion);
        if(find_cache_bin(req->thr_arg, cache_key, &buff_ptr, &img_size) == 1)
        {
            LOG_PRINT(LOG_DEBUG, "Hit Color Image Cache[Key: %s, len: %d].", cache_key, img_size);
            status = MagickReadImageBlob(magick_wand, buff_ptr, img_size);
            free(buff_ptr);
            buff_ptr = NULL;
            if(status == MagickFalse)
            {
                LOG_PRINT(LOG_DEBUG, "Color Image Cache[Key: %s] is Bad. Remove.", cache_key);
                del_cache(req->thr_arg, cache_key);
            }
            else
            {
                got_color = true;
                LOG_PRINT(LOG_DEBUG, "Read Image from Color Image Cache[Key: %s, len: %d] Succ. Goto Convert.", cache_key, img_size);
                goto convert;
            }
        }
        if(get_img_db(req->thr_arg, cache_key, &buff_ptr, &img_size) == 1)
        {
            LOG_PRINT(LOG_DEBUG, "Get color image [%s] from backend db.", cache_key);
            status = MagickReadImageBlob(magick_wand, buff_ptr, img_size);
            if(status == MagickTrue)
            {
                got_color = true;
                LOG_PRINT(LOG_DEBUG, "Read Image from Color Image[%s] Succ. Goto Convert.", cache_key);
                if(img_size < CACHE_MAX_SIZE)
                {
                    set_cache_bin(req->thr_arg, cache_key, buff_ptr, img_size);
                }
                free(buff_ptr);
                buff_ptr = NULL;
                goto convert;
            }
        }
    }

    gen_key(cache_key, req->md5, 0);
    if(find_cache_bin(req->thr_arg, cache_key, &buff_ptr, &img_size) == 1)
    {
        LOG_PRINT(LOG_DEBUG, "Hit Cache[Key: %s].", cache_key);
    }
    else
    {
        if(get_img_db(req->thr_arg, cache_key, &buff_ptr, &img_size) == -1)
        {
            LOG_PRINT(LOG_DEBUG, "Get image [%s] from backend db failed.", cache_key);
            goto err;
        }
        else if(img_size < CACHE_MAX_SIZE)
        {
            set_cache_bin(req->thr_arg, cache_key, buff_ptr, img_size);
        }
    }

    status = MagickReadImageBlob(magick_wand, buff_ptr, img_size);
    free(buff_ptr);
    buff_ptr = NULL;
    if(status == MagickFalse)
    {
        ThrowWandException(magick_wand);
        del_cache(req->thr_arg, cache_key);
        LOG_PRINT(LOG_DEBUG, "Read image [%s] from blob failed.", cache_key);
        goto err;
    }

    if(req->width == 0 && req->height == 0)
    {
        LOG_PRINT(LOG_DEBUG, "Image [%s] needn't resize. Goto Convert.", cache_key);
        goto convert;
    }

    int width, height;
    width = req->width;
    height = req->height;
    float owidth = MagickGetImageWidth(magick_wand);
    float oheight = MagickGetImageHeight(magick_wand);
    if(width <= owidth && height <= oheight)
    {
        if(req->proportion == 1 || (req->proportion == 0 && req->width * req->height == 0))
        {
            if(req->height == 0)
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
            LOG_PRINT(LOG_DEBUG, "Image[%s] Resize Failed!", cache_key);
            goto err;
        }

        LOG_PRINT(LOG_DEBUG, "Resize img succ.");
    }
    else
    {
        got_rsp = true;
        LOG_PRINT(LOG_DEBUG, "Args width/height is bigger than real size, return original image.");
    }

convert:

    //compress image
    if(got_color == false)
    {
        LOG_PRINT(LOG_DEBUG, "Start to Compress the Image!");
        img_format = MagickGetImageFormat(magick_wand);
        LOG_PRINT(LOG_DEBUG, "Image Format is %s", img_format);
        if(strcmp(img_format, "JPEG") != 0)
        {
            LOG_PRINT(LOG_DEBUG, "Convert Image Format from %s to JPEG.", img_format);
            status = MagickSetImageFormat(magick_wand, "JPEG");
            if(status == MagickFalse)
            {
                LOG_PRINT(LOG_DEBUG, "Image[%s] Convert Format Failed!", cache_key);
            }
            LOG_PRINT(LOG_DEBUG, "Compress Image with JPEGCompression");
            status = MagickSetImageCompression(magick_wand, JPEGCompression);
            if(status == MagickFalse)
            {
                LOG_PRINT(LOG_DEBUG, "Image[%s] Compression Failed!", cache_key);
            }
        }
        size_t quality = MagickGetImageCompressionQuality(magick_wand);
        LOG_PRINT(LOG_DEBUG, "Image Compression Quality is %u.", quality);
        if(quality > WAP_QUALITY)
        {
            quality = WAP_QUALITY;
        }
        LOG_PRINT(LOG_DEBUG, "Set Compression Quality to 75%.");
        status = MagickSetImageCompressionQuality(magick_wand, quality);
        if(status == MagickFalse)
        {
            LOG_PRINT(LOG_DEBUG, "Set Compression Quality Failed!");
        }

        //strip image EXIF infomation
        LOG_PRINT(LOG_DEBUG, "Start to Remove Exif Infomation of the Image...");
        status = MagickStripImage(magick_wand);
        if(status == MagickFalse)
        {
            LOG_PRINT(LOG_DEBUG, "Remove Exif Infomation of the ImageFailed!");
        }

        buff_ptr = (char *)MagickGetImageBlob(magick_wand, &img_size);
        if(buff_ptr == NULL)
        {
            LOG_PRINT(LOG_DEBUG, "Magick Get Image Blob Failed!");
            goto err;
        }
        gen_key(cache_key, req->md5, 3, req->width, req->height, req->proportion);
        if(settings.save_new == 1 && got_rsp == false)
        {
            save_img_db(req->thr_arg, cache_key, buff_ptr, img_size);
        }
        if(img_size < CACHE_MAX_SIZE)
        {
            set_cache_bin(req->thr_arg, cache_key, buff_ptr, img_size);
        }
        if(req->gray == 1)
        {
            free(buff_ptr);
            buff_ptr = NULL;
        }
        else
        {
            goto done;
        }
    } 

    //gray image
    if(req->gray == 1)
    {
        LOG_PRINT(LOG_DEBUG, "Start to Remove Color!");
        status = MagickSetImageColorspace(magick_wand, GRAYColorspace);
        if(status == MagickFalse)
        {
            LOG_PRINT(LOG_DEBUG, "Image[%s] Remove Color Failed!", cache_key);
            goto err;
        }
        LOG_PRINT(LOG_DEBUG, "Image Remove Color Finish!");


        buff_ptr = (char *)MagickGetImageBlob(magick_wand, &img_size);
        if(buff_ptr == NULL)
        {
            LOG_PRINT(LOG_DEBUG, "Magick Get Image Blob Failed!");
            goto err;
        }
        gen_key(cache_key, req->md5, 4, req->width, req->height, req->proportion, req->gray);
        if(settings.save_new == 1 && got_rsp == false)
        {
            save_img_db(req->thr_arg, cache_key, buff_ptr, img_size);
        }
        if(img_size < CACHE_MAX_SIZE)
        {
            set_cache_bin(req->thr_arg, cache_key, buff_ptr, img_size);
        }
    }

done:
    if(settings.etag == 1)
    {
        result = zimg_etag_set(request, buff_ptr, img_size);
        if(result == 2)
            goto err;
    }
    result = evbuffer_add(request->buffer_out, buff_ptr, img_size);
    if(result != -1)
    {
        result = 1;
    }

err:
    if(magick_wand)
    {
        magick_wand=DestroyMagickWand(magick_wand);
    }
    if(img_format)
        free(img_format);
    if(buff_ptr)
        free(buff_ptr);
    return result;
}

int get_img_mode_db2(zimg_req_t *req, evhtp_request_t *request)
{
    int result = -1;
    char cache_key[CACHE_KEY_SIZE];
    char *buff_ptr = NULL;
    size_t img_size;
    struct image *im = NULL;
    bool to_save = true;

    LOG_PRINT(LOG_DEBUG, "get_img() start processing zimg request...");

    gen_key(cache_key, req->md5, 0);
    if(exist_db(req->thr_arg, cache_key) == -1)
    {
        LOG_PRINT(LOG_DEBUG, "Image [%s] is not existed.", cache_key);
        goto err;
    }

    if(settings.script_on == 1 && req->type != NULL)
    {
        gen_key(cache_key, req->md5, 1, req->type);
    }
    else
    {
        if(req->x != 0 || req->y != 0)
            req->proportion = 1;

        if(!(req->proportion == 0 && req->width == 0 && req->height == 0))
        {
            gen_key(cache_key, req->md5, 7, req->width, req->height, req->proportion, req->gray, req->x, req->y, req->quality);
        }
    }

    if(find_cache_bin(req->thr_arg, cache_key, &buff_ptr, &img_size) == 1)
    {
        LOG_PRINT(LOG_DEBUG, "Hit Cache[Key: %s].", cache_key);
        to_save = false;
        goto done;
    }
    LOG_PRINT(LOG_DEBUG, "Start to Find the Image...");
    if(get_img_db(req->thr_arg, cache_key, &buff_ptr, &img_size) == 1)
    {
        LOG_PRINT(LOG_DEBUG, "Get image [%s] from backend db succ.", cache_key);
        if(img_size < CACHE_MAX_SIZE)
        {
            set_cache_bin(req->thr_arg, cache_key, buff_ptr, img_size);
        }
        to_save = false;
        goto done;
    }

    im = wi_new_image();
    if (im == NULL) goto err;

    gen_key(cache_key, req->md5, 0);
    if(find_cache_bin(req->thr_arg, cache_key, &buff_ptr, &img_size) == -1)
    {
        if(get_img_db(req->thr_arg, cache_key, &buff_ptr, &img_size) == -1)
        {
            LOG_PRINT(LOG_DEBUG, "Get image [%s] from backend db failed.", cache_key);
            goto err;
        }
        else if(img_size < CACHE_MAX_SIZE)
        {
            set_cache_bin(req->thr_arg, cache_key, buff_ptr, img_size);
        }
    }

    result = wi_read_blob(im, buff_ptr, img_size);
    if (result != WI_OK)
    {
        LOG_PRINT(LOG_DEBUG, "Webimg Read Blob Failed!");
        goto err;
    }
    if(settings.script_on == 1 && req->type != NULL)
        result = lua_convert(im, req);
    else
        result = convert(im, req);
    if(result == -1) goto err;
    if(result == 1) to_save = false;

    buff_ptr = (char *)wi_get_blob(im, &img_size);
    if (buff_ptr == NULL) {
        LOG_PRINT(LOG_DEBUG, "Webimg Get Blob Failed!");
        goto err;
    }

    if(settings.script_on == 1 && req->type != NULL)
        gen_key(cache_key, req->md5, 1, req->type);
    else
        gen_key(cache_key, req->md5, 7, req->width, req->height, req->proportion, req->gray, req->x, req->y, req->quality);
    if(img_size < CACHE_MAX_SIZE)
    {
        set_cache_bin(req->thr_arg, cache_key, buff_ptr, img_size);
    }

done:
    if(settings.etag == 1)
    {
        result = zimg_etag_set(request, buff_ptr, img_size);
        if(result == 2)
            goto err;
    }
    result = evbuffer_add(request->buffer_out, buff_ptr, img_size);
    if(result != -1)
    {
        if(settings.save_new != 0 && to_save == true)
        {
            if(settings.save_new == 1 || (settings.save_new == 2 && req->type != NULL)) 
            {
                LOG_PRINT(LOG_DEBUG, "Image [%s] Saved to Storage.", cache_key);
                save_img_db(req->thr_arg, cache_key, buff_ptr, img_size);
            }
            else
                LOG_PRINT(LOG_DEBUG, "Image [%s] Needn't to Storage.", cache_key);
        }
        else
            LOG_PRINT(LOG_DEBUG, "Image [%s] Needn't to Storage.", cache_key);
        result = 1;
    }

err:
    if(im != NULL)
        wi_free_image(im);
    else if(buff_ptr != NULL)
        free(buff_ptr);
    return result;
}

/**
 * @brief get_img_db Choose db to get image by setting.
 *
 * @param thr_arg Thread arg struct.
 * @param cache_key The key.
 * @param buff Image buffer.
 * @param len Image size.
 *
 * @return 1 for succ or -1 for failed.
 */
int get_img_db(thr_arg_t *thr_arg, const char *cache_key, char **buff, size_t *len)
{
    int ret = -1;

    if(settings.mode == 2 && thr_arg->beansdb_conn != NULL)
        ret = get_img_beansdb(thr_arg->beansdb_conn, cache_key, buff, len);
    else if(settings.mode == 3 && thr_arg->ssdb_conn != NULL)
        ret = get_img_ssdb(thr_arg->ssdb_conn, cache_key, buff, len);

    if(ret == -1 && settings.mode == 3)
    {
        if(thr_arg->ssdb_conn != NULL)
            redisFree(thr_arg->ssdb_conn);
        redisContext* c = redisConnect(settings.ssdb_ip, settings.ssdb_port);
        if(c->err)
        {
            redisFree(c);
            LOG_PRINT(LOG_DEBUG, "Connect to ssdb server faile");
            return ret;
        }
        else
        {
            thr_arg->ssdb_conn = c;
            LOG_PRINT(LOG_DEBUG, "SSDB Server Reconnected.");
            ret = get_img_ssdb(thr_arg->ssdb_conn, cache_key, buff, len);
            //evthr_set_aux(thr_arg->thread, thr_arg);
        }
    }
    return ret;
}

/**
* @brief get_img_beansdb Find a key's BINARY value from beansdb backend.
*
* @param memc The connection to beansdb.
* @param key The key you want to find.
* @param value_ptr It will be alloc and contains the binary value.
* @param len It will change to the length of the value.
*
* @return 1 for success and -1 for fail.
*/
int get_img_beansdb(memcached_st *memc, const char *key, char **value_ptr, size_t *len)
{
    int rst = -1;
    if(memc == NULL)
        return rst;

    uint32_t  flags;
    memcached_return rc;

    *value_ptr = memcached_get(memc, key, strlen(key), len, &flags, &rc);

    if (rc == MEMCACHED_SUCCESS) 
    {
        LOG_PRINT(LOG_DEBUG, "Binary Beansdb Find Key[%s], Len: %d.", key, *len);
        rst = 1;
    }
    else if (rc == MEMCACHED_NOTFOUND)
    {
        LOG_PRINT(LOG_DEBUG, "Binary Beansdb Key[%s] Not Find!", key);
        rst = -1;
    }
    else
    {
        const char *str_rc = memcached_strerror(memc, rc);
        LOG_PRINT(LOG_DEBUG, "Beansdb Result: %s", str_rc);
    }

    return rst;
}

/**
 * @brief get_img_ssdb Get image from ssdb backend by key
 *
 * @param c Conntion to ssdb.
 * @param cache_key image key
 * @param buff buffer to storage image
 * @param len image size
 *
 * @return 1 for success and -1 for fail
 */
int get_img_ssdb(redisContext* c, const char *cache_key, char **buff, size_t *len)
{
    if(c == NULL)
        return -1;

    redisReply *r = (redisReply*)redisCommand(c, "GET %s", cache_key);
    if( NULL == r)
    {
        LOG_PRINT(LOG_DEBUG, "Execut ssdb command failure");
        return -1;
    }
    if( r->type != REDIS_REPLY_STRING )
    {
        LOG_PRINT(LOG_DEBUG, "Failed to execute get [%s] from ssdb.", cache_key);
        freeReplyObject(r);
        return -1;
    }

    *len = r->len;
    *buff = (char *)malloc(r->len);
    if(buff == NULL)
    {
        LOG_PRINT(LOG_DEBUG, "buff malloc failed!");
        return -1;
    }
    memcpy(*buff, r->str, r->len);

    freeReplyObject(r);
    LOG_PRINT(LOG_DEBUG, "Succeed to get [%s] from ssdb. length = [%d].", cache_key, *len);

    return 1;
}


/**
 * @brief save_img_db Choose db mode to save images.
 *
 * @param thr_arg Thread arg struct.
 * @param cache_key The key of image.
 * @param buff Image buffer.
 * @param len Image size.
 *
 * @return 1 for succ or -1 for fialed.
 */
int save_img_db(thr_arg_t *thr_arg, const char *cache_key, const char *buff, const size_t len)
{
    int ret = -1;
    if(settings.mode == 2)
        ret = save_img_beansdb(thr_arg->beansdb_conn, cache_key, buff, len);
    else if(settings.mode == 3)
        ret = save_img_ssdb(thr_arg->ssdb_conn, cache_key, buff, len);

    if(ret == -1 && settings.mode == 3)
    {
        if(thr_arg->ssdb_conn != NULL)
            redisFree(thr_arg->ssdb_conn);
        redisContext* c = redisConnect(settings.ssdb_ip, settings.ssdb_port);
        if(c->err)
        {
            redisFree(c);
            LOG_PRINT(LOG_DEBUG, "Connect to ssdb server faile");
            return ret;
        }
        else
        {
            thr_arg->ssdb_conn = c;
            LOG_PRINT(LOG_DEBUG, "SSDB Server Reconnected.");
            ret = save_img_ssdb(thr_arg->ssdb_conn, cache_key, buff, len);
            //evthr_set_aux(thr_arg->thread, thr_arg);
        }
    }
    return ret;
}

/**
* @brief save_img_beansdb Set a new BINARY value of a key in beansdb.
*
* @param memc The connection to beansdb.
* @param key The key.
* @param value A char * buffer you want to set.
* @param len The length of the buffer above,
*
* @return 1 for success and -1 for fial.
*/
int save_img_beansdb(memcached_st *memc, const char *key, const char *value, const size_t len)
{
    int rst = -1;
    if(memc == NULL)
        return rst;

    memcached_return rc;

    rc = memcached_set(memc, key, strlen(key), value, len, 0, 0);

    if (rc == MEMCACHED_SUCCESS) 
    {
        LOG_PRINT(LOG_DEBUG, "Binary Beansdb Set Successfully. Key[%s] Len: %d.", key, len);
        rst = 1;
    }
    else
    {
        LOG_PRINT(LOG_DEBUG, "Binary beansdb Set Key: [%s] Failed!", key);
        const char *str_rc = memcached_strerror(memc, rc);
        LOG_PRINT(LOG_DEBUG, "Beansdb Result: %s", str_rc);
        rst = -1;
    }

    return rst;
}

/**
 * @brief save_img_ssdb Save image to ssdb backends.
 *
 * @param c Conntion to ssdb.
 * @param cache_key image key
 * @param buff image buff
 * @param len image buff length
 *
 * @return 1 for success and -1 for fail
 */
int save_img_ssdb(redisContext* c, const char *cache_key, const char *buff, const size_t len)
{
    if(c == NULL)
        return -1;

    redisReply *r = (redisReply*)redisCommand(c, "SET %s %b", cache_key, buff, len);
    if( NULL == r)
    {
        LOG_PRINT(LOG_DEBUG, "Execut ssdb command failure");
        return -1;
    }
    if( !(r->type == REDIS_REPLY_STATUS && strcasecmp(r->str,"OK")==0))
    {
        LOG_PRINT(LOG_DEBUG, "Failed to execute save [%s] to ssdb.", cache_key);
        freeReplyObject(r);
        return -1;
    }
    freeReplyObject(r);
    LOG_PRINT(LOG_DEBUG, "Succeed to save [%s] to ssdb. length = [%d].", cache_key, len);

    return 1;
}

int admin_img_mode_db(thr_arg_t *thr_arg, char *md5, int t)
{
    int result = -1;

    LOG_PRINT(LOG_DEBUG, "admin_img_mode_db() start processing admin request...");

    char cache_key[CACHE_KEY_SIZE];
    gen_key(cache_key, md5, 0);
    LOG_PRINT(LOG_DEBUG, "original key: %s", cache_key);

    result = exist_db(thr_arg, cache_key);
    if(result == -1)
        return 2;

    if(t == 1)
    {
        result = del_db(thr_arg, cache_key);
    }
    return result;
}

int exist_db(thr_arg_t *thr_arg, const char *cache_key)
{
    int result = -1;
    if(settings.mode == 2)
    {
        if(exist_beansdb(thr_arg->beansdb_conn, cache_key) == 1)
            result = 1;
        else
        {
            LOG_PRINT(LOG_DEBUG, "key: %s is not exist!", cache_key);
        }
    }
    else if(settings.mode == 3)
    {
        if(exist_ssdb(thr_arg->ssdb_conn, cache_key) == 1)
            result = 1;
        else
        {
            LOG_PRINT(LOG_DEBUG, "key: %s is not exist!", cache_key);
        }
    }
    return result;
}

/**
 * @brief exist_beansdb If a key is existed in beansdb.
 *
 * @param memc The connection to beansdb.
 * @param key Key
 *
 * @return 1 for success and -1 for fail
 */
int exist_beansdb(memcached_st *memc, const char *key)
{
    /*
    int rst = -1;
    if(memc == NULL)
        return rst;

    memcached_return rc;

    //memcached_get(memc, key, strlen(key), &valueLen, &flags, &rc);
    rc = memcached_exist(memc, key, strlen(key));

    if (rc == MEMCACHED_SUCCESS) 
    {
        LOG_PRINT(LOG_DEBUG, "Beansdb Key[%s] Exist.", key);
        rst = 1;
    }
    else
    {
        const char *str_rc = memcached_strerror(memc, rc);
        LOG_PRINT(LOG_DEBUG, "Beansdb Result: %s", str_rc);
    }

    return rst;
    */
    int rst = -1;
    if(memc == NULL)
        return rst;

    size_t valueLen;
    uint32_t flags;
    memcached_return rc;

    memcached_get(memc, key, strlen(key), &valueLen, &flags, &rc);

    if (rc == MEMCACHED_SUCCESS) 
    {
        LOG_PRINT(LOG_DEBUG, "Beansdb Key[%s] Exist.", key);
        rst = 1;
    }
    else
    {
        const char *str_rc = memcached_strerror(memc, rc);
        LOG_PRINT(LOG_DEBUG, "Beansdb Result: %s", str_rc);
    }

    return rst;
}

int exist_ssdb(redisContext* c, const char *cache_key)
{
    int rst = -1;
    if(c == NULL)
        return rst;

    redisReply *r = (redisReply*)redisCommand(c, "EXISTS %s", cache_key);
    if (r && r->type != REDIS_REPLY_NIL && r->type == REDIS_REPLY_INTEGER && r->integer == 1)
    {
        LOG_PRINT(LOG_DEBUG, "ssdb key %s exist %d", cache_key, r->integer);
        rst = 1;
    }
    
    freeReplyObject(r);
    return rst;
}

int del_db(thr_arg_t *thr_arg, const char *cache_key)
{
    int result = -1;
    if(settings.mode == 2)
    {
        if(del_beansdb(thr_arg->beansdb_conn, cache_key) == -1)
        {
            LOG_PRINT(LOG_DEBUG, "delete key: %s failed!", cache_key);
        }
        else
            result = 1;
    }
    else if(settings.mode == 3)
    {
        if(del_ssdb(thr_arg->ssdb_conn, cache_key) == -1)
        {
            LOG_PRINT(LOG_DEBUG, "delete key: %s failed!", cache_key);
        }
        else
            result = 1;
    }
    return result;
}

/**
* @brief del_beansdb This function delete a key and its value in beansdb.
*
* @param memc The connection to beansdb.
* @param key The key.
*
* @return 1 for success and -1 for fail.
*/
int del_beansdb(memcached_st *memc, const char *key)
{
    int rst = -1;
    if(memc == NULL)
        return rst;

    memcached_return rc;

    rc = memcached_delete(memc, key, strlen(key), 0);

    if (rc == MEMCACHED_SUCCESS) 
    {
        LOG_PRINT(LOG_DEBUG, "Beansdb Key[%s] Delete Successfully.", key);
        rst = 1;
    }
    else
    {
        LOG_PRINT(LOG_DEBUG, "Beansdb Key[%s] Delete Failed!", key);
        const char *str_rc = memcached_strerror(memc, rc);
        LOG_PRINT(LOG_DEBUG, "Beansdb Result: %s", str_rc);
        rst = -1;
    }

    return rst;
}

int del_ssdb(redisContext* c, const char *cache_key)
{
    int rst = -1;
    if(c == NULL)
        return rst;

    redisReply *r = (redisReply*)redisCommand(c, "DEL %s", cache_key);
    if (r && r->type != REDIS_REPLY_NIL && r->type == REDIS_REPLY_INTEGER && r->integer == 1)
    {
        LOG_PRINT(LOG_DEBUG, "ssdb key %s deleted %d", cache_key, r->integer);
        rst = 1;
    }
    
    freeReplyObject(r);
    return rst;
}



/**
 * @brief find_beansdb Connect to a beansdb server and find a key's value.
 *
 * @param memc The connection to beansdb.
 * @param key The string of the key you want to find.
 * @param value It contains the string of the key's value.
 *
 * @return 1 for success and -1 for fail.
 */
int find_beansdb(memcached_st *memc, const char *key, char *value)
{
    //LOG_PRINT(LOG_INFO, "Beansdb Find Key[%s] Value Size: %d", key, sizeof(value));
    int rst = -1;
    if(memc == NULL)
        return rst;

    size_t valueLen;
    uint32_t  flags;
    memcached_return rc;

    char *pvalue = memcached_get(memc, key, strlen(key), &valueLen, &flags, &rc);

    if (rc == MEMCACHED_SUCCESS) 
    {
        LOG_PRINT(LOG_DEBUG, "Beansdb Find Key[%s] Value: %s", key, pvalue);
        str_lcpy(value, pvalue, sizeof(value));
        free(pvalue);
        rst = 1;
    }
    else if (rc == MEMCACHED_NOTFOUND)
    {
        LOG_PRINT(LOG_DEBUG, "Beansdb Key[%s] Not Find!", key);
        rst = -1;
    }
    else
    {
        const char *str_rc = memcached_strerror(memc, rc);
        LOG_PRINT(LOG_DEBUG, "Beansdb Result: %s", str_rc);
    }

    return rst;
}


/**
 * @brief set_beansdb Set a key with the value input.
 *
 * @param memc The connection to beansdb.
 * @param key The key you want to set a new value.
 * @param value The value of the key.
 *
 * @return 1 for success and -1 for fail.
 */
int set_beansdb(memcached_st *memc, const char *key, const char *value)
{
    int rst = -1;
    if(memc == NULL)
        return rst;

    memcached_return rc;

    rc = memcached_set(memc, key, strlen(key), value, strlen(value), 0, 0);

    if (rc == MEMCACHED_SUCCESS) 
    {
        LOG_PRINT(LOG_DEBUG, "Beansdb Set Successfully. Key[%s] Value: %s", key, value);
        rst = 1;
    }
    else
    {
        LOG_PRINT(LOG_DEBUG, "Beansdb Set(Key: %s Value: %s) Failed!", key, value);
        const char *str_rc = memcached_strerror(memc, rc);
        LOG_PRINT(LOG_DEBUG, "Beansdb Result: %s", str_rc);
        rst = -1;
    }

    return rst;
}
