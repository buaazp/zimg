/*
 *   zimg - high performance image storage and processing system.
 *       http://zimg.buaa.us
 *
 *   Copyright (c) 2013, Peter Zhao <zp@buaa.us>.
 *   All rights reserved.
 *
 *   Use and distribution licensed under the BSD license.
 *   See the LICENSE file for full text.
 *
 */


/**
 * @file zimg.c
 * @brief Convert, get and save image functions.
 * @author 招牌疯子 zp@buaa.us
 * @version 1.0
 * @date 2013-07-19
 */

#include <stdio.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <wand/MagickWand.h>
#include "webimg/webimg2.h"
#include "zimg.h"
#include "zmd5.h"
#include "zlog.h"
#include "zcache.h"
#include "zutil.h"
#include "zdb.h"
#include "zscale.h"
#include "zhttpd.h"

extern struct setting settings;

int save_img(thr_arg_t *thr_arg, const char *buff, const int len, char *md5);
int new_img(const char *buff, const size_t len, const char *save_name);
int get_img(zimg_req_t *req, evhtp_request_t *request);
int get_img2(zimg_req_t *req, evhtp_request_t *request);


/**
 * @brief save_img Save buffer from POST requests
 *
 * @param thr_arg Thread arg struct.
 * @param buff The char * from POST request
 * @param len The length of buff
 * @param md5 Parsed md5 from url
 *
 * @return 1 for success and -1 for fail
 */
int save_img(thr_arg_t *thr_arg, const char *buff, const int len, char *md5)
{
    int result = -1;

    LOG_PRINT(LOG_DEBUG, "Begin to Caculate MD5...");
    md5_state_t mdctx;
    md5_byte_t md_value[16];
    char md5sum[33];
    int i;
    int h, l;
    md5_init(&mdctx);
    md5_append(&mdctx, (const unsigned char*)(buff), len);
    md5_finish(&mdctx, md_value);

    for(i=0; i<16; ++i)
    {
        h = md_value[i] & 0xf0;
        h >>= 4;
        l = md_value[i] & 0x0f;
        md5sum[i * 2] = (char)((h >= 0x0 && h <= 0x9) ? (h + 0x30) : (h + 0x57));
        md5sum[i * 2 + 1] = (char)((l >= 0x0 && l <= 0x9) ? (l + 0x30) : (l + 0x57));
    }
    md5sum[32] = '\0';
    strcpy(md5, md5sum);
    LOG_PRINT(LOG_DEBUG, "md5: %s", md5sum);

    char cache_key[CACHE_KEY_SIZE];
    char save_path[512];
    char save_name[512];
    gen_key(cache_key, md5sum, 0);
    if(exist_cache(thr_arg, cache_key) == 1)
    {
        LOG_PRINT(LOG_DEBUG, "File Exist, Needn't Save.");
        result = 1;
        goto done;
    }
    LOG_PRINT(LOG_DEBUG, "exist_cache not found. Begin to Save File.");


    if(settings.mode != 1)
    {
        if(save_img_db(thr_arg, cache_key, buff, len) == -1)
        {
            LOG_PRINT(LOG_DEBUG, "save_img_db failed.");
            goto done;
        }
        else
        {
            LOG_PRINT(LOG_DEBUG, "save_img_db succ.");
            result = 1;
            goto done;
        }
    }

    //caculate 2-level path
    int lvl1 = str_hash(md5sum);
    int lvl2 = str_hash(md5sum + 3);

    snprintf(save_path, 512, "%s/%d/%d/%s", settings.img_path, lvl1, lvl2, md5sum);
    LOG_PRINT(LOG_DEBUG, "save_path: %s", save_path);

    if(is_dir(save_path) == 1)
    {
        LOG_PRINT(LOG_DEBUG, "Check File Exist. Needn't Save.");
        goto cache;
    }

    if(mk_dirs(save_path) == -1)
    {
        LOG_PRINT(LOG_DEBUG, "save_path[%s] Create Failed!", save_path);
        goto done;
    }
    LOG_PRINT(LOG_DEBUG, "save_path[%s] Create Finish.", save_path);
    snprintf(save_name, 512, "%s/0*0", save_path);
    LOG_PRINT(LOG_DEBUG, "save_name-->: %s", save_name);
    if(new_img(buff, len, save_name) == -1)
    {
        LOG_PRINT(LOG_DEBUG, "Save Image[%s] Failed!", save_name);
        goto done;
    }

cache:
    if(len < CACHE_MAX_SIZE)
    {
        gen_key(cache_key, md5sum, 0);
        set_cache_bin(thr_arg, cache_key, buff, len);
    }
    result = 1;

done:
    return result;
}

/**
 * @brief new_img The real function to save a image to disk.
 *
 * @param buff Const buff to write to disk.
 * @param len The length of buff.
 * @param save_name The name you want to save.
 *
 * @return 1 for success and -1 for fail.
 */
int new_img(const char *buff, const size_t len, const char *save_name)
{
    int result = -1;
    LOG_PRINT(LOG_DEBUG, "Start to Storage the New Image...");
    int fd = -1;
    int wlen = 0;

    if((fd = open(save_name, O_WRONLY | O_TRUNC | O_CREAT, 00644)) < 0)
    {
        LOG_PRINT(LOG_DEBUG, "fd(%s) open failed!", save_name);
        goto done;
    }

    if(flock(fd, LOCK_EX | LOCK_NB) == -1)
    {
        LOG_PRINT(LOG_DEBUG, "This fd is Locked by Other thread.");
        goto done;
    }

    wlen = write(fd, buff, len);
    if(wlen == -1)
    {
        LOG_PRINT(LOG_DEBUG, "write(%s) failed!", save_name);
        goto done;
    }
    else if(wlen < len)
    {
        LOG_PRINT(LOG_DEBUG, "Only part of [%s] is been writed.", save_name);
        goto done;
    }
    flock(fd, LOCK_UN | LOCK_NB);
    LOG_PRINT(LOG_DEBUG, "Image [%s] Write Successfully!", save_name);
    result = 1;

done:
    if(fd != -1)
    {
        close(fd);
    }
    return result;
}

/* get image method used for zimg servise, such as:
 * http://127.0.0.1:4869/c6c4949e54afdb0972d323028657a1ef?w=100&h=50&p=1&g=1
 */

/**
 * @brief get_img The function of getting a image buffer and it's length.
 *
 * @param req The zimg_req_t from zhttp and it has the params of a request.
 * @param buff_ptr This function return image buffer in it.
 * @param img_size Get_img will change this number to return the size of image buffer.
 *
 * @return 1 for success and -1 for fail.
 */
int get_img(zimg_req_t *req, evhtp_request_t *request)
{
    int result = -1;
    char cache_key[CACHE_KEY_SIZE];
    char *img_format = NULL;
    int fd = -1;
    struct stat f_stat;
    char *buff_ptr;
    size_t img_size;

    MagickBooleanType status;
    MagickWand *magick_wand = NULL;

    bool got_rsp = true;
    bool got_color = false;

    LOG_PRINT(LOG_DEBUG, "get_img() start processing zimg request...");

    // to gen cache_key like this: 926ee2f570dc50b2575e35a6712b08ce:0:0:1:0
    gen_key(cache_key, req->md5, 4, req->width, req->height, req->proportion, req->gray);
    if(find_cache_bin(req->thr_arg, cache_key, &buff_ptr, &img_size) == 1)
    {
        LOG_PRINT(LOG_DEBUG, "Hit Cache[Key: %s].", cache_key);
        goto done;
    }
    LOG_PRINT(LOG_DEBUG, "Start to Find the Image...");

    char whole_path[512];
    int lvl1 = str_hash(req->md5);
    int lvl2 = str_hash(req->md5 + 3);
    snprintf(whole_path, 512, "%s/%d/%d/%s", settings.img_path, lvl1, lvl2, req->md5);
    LOG_PRINT(LOG_DEBUG, "docroot: %s", settings.img_path);
    LOG_PRINT(LOG_DEBUG, "req->md5: %s", req->md5);
    LOG_PRINT(LOG_DEBUG, "whole_path: %s", whole_path);

    char name[128];
    if(req->proportion && req->gray)
        snprintf(name, 128, "%d*%dpg", req->width, req->height);
    else if(req->proportion && !req->gray)
        snprintf(name, 128, "%d*%dp", req->width, req->height);
    else if(!req->proportion && req->gray)
        snprintf(name, 128, "%d*%dg", req->width, req->height);
    else
        snprintf(name, 128, "%d*%d", req->width, req->height);

    char color_name[128];
    if(req->proportion)
        snprintf(color_name, 128, "%d*%dp", req->width, req->height);
    else
        snprintf(color_name, 128, "%d*%d", req->width, req->height);

    char orig_path[512];
    snprintf(orig_path, strlen(whole_path) + 6, "%s/0*0", whole_path);
    LOG_PRINT(LOG_DEBUG, "0rig File Path: %s", orig_path);

    char rsp_path[512];
    if(req->width == 0 && req->height == 0 && req->proportion == 0 && req->gray == 0)
    {
        LOG_PRINT(LOG_DEBUG, "Return original image.");
        strncpy(rsp_path, orig_path, 512);
    }
    else
    {
        snprintf(rsp_path, 512, "%s/%s", whole_path, name);
    }
    LOG_PRINT(LOG_DEBUG, "Got the rsp_path: %s", rsp_path);

    char color_path[512];
    snprintf(color_path, 512, "%s/%s", whole_path, color_name);


    //status=MagickReadImage(magick_wand, rsp_path);
    if((fd = open(rsp_path, O_RDONLY)) == -1)
    {
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

            status=MagickReadImage(magick_wand, color_path);
            if(status == MagickTrue)
            {
                got_color = true;
                LOG_PRINT(LOG_DEBUG, "Read Image from Color Image[%s] Succ. Goto Convert.", rsp_path);
                buff_ptr = (char *)MagickGetImageBlob(magick_wand, &img_size);
                if(img_size < CACHE_MAX_SIZE)
                {
                    set_cache_bin(req->thr_arg, cache_key, buff_ptr, img_size);
                }
                free(buff_ptr);
                buff_ptr = NULL;

                goto convert;
            }
        }

        // to gen cache_key like this: rsp_path-/926ee2f570dc50b2575e35a6712b08ce
        gen_key(cache_key, req->md5, 0);
        if(find_cache_bin(req->thr_arg, cache_key, &buff_ptr, &img_size) == 1)
        {
            LOG_PRINT(LOG_DEBUG, "Hit Orignal Image Cache[Key: %s].", cache_key);
            status = MagickReadImageBlob(magick_wand, buff_ptr, img_size);
            free(buff_ptr);
            buff_ptr = NULL;
            if(status == MagickFalse)
            {
                LOG_PRINT(LOG_DEBUG, "Open Original Image From Blob Failed! Begin to Open it From Disk.");
                ThrowWandException(magick_wand);
                del_cache(req->thr_arg, cache_key);
                status = MagickReadImage(magick_wand, orig_path);
                if(status == MagickFalse)
                {
                    ThrowWandException(magick_wand);
                    goto err;
                }
                else
                {
                    buff_ptr = (char *)MagickGetImageBlob(magick_wand, &img_size);
                    if(img_size < CACHE_MAX_SIZE)
                    {
                        set_cache_bin(req->thr_arg, cache_key, buff_ptr, img_size);
                    }
                    free(buff_ptr);
                    buff_ptr = NULL;
                }
            }
        }
        else
        {
            LOG_PRINT(LOG_DEBUG, "Not Hit Original Image Cache. Begin to Open it.");
            status = MagickReadImage(magick_wand, orig_path);
            if(status == MagickFalse)
            {
                ThrowWandException(magick_wand);
                goto err;
            }
            else
            {
                buff_ptr = (char *)MagickGetImageBlob(magick_wand, &img_size);
                if(img_size < CACHE_MAX_SIZE)
                {
                    set_cache_bin(req->thr_arg, cache_key, buff_ptr, img_size);
                }
                free(buff_ptr);
                buff_ptr = NULL;
            }
        }

        int width, height;
        width = req->width;
        height = req->height;
        if(width == 0 && height == 0)
        {
            LOG_PRINT(LOG_DEBUG, "Image[%s] needn't resize. Goto Convert.", orig_path);
            goto convert;
        }
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
                LOG_PRINT(LOG_DEBUG, "Image[%s] Resize Failed!", orig_path);
                goto err;
            }
            LOG_PRINT(LOG_DEBUG, "Resize img succ.");
        }
        else
        {
            // Note this strcpy because rsp_path is not useful. We needn't to save the new image.
            got_rsp = true;
            LOG_PRINT(LOG_DEBUG, "Args width/height is bigger than real size, return original image.");
        }
    }
    else
    {
        fstat(fd, &f_stat);
        size_t rlen = 0;
        img_size = f_stat.st_size;
        if(img_size <= 0)
        {
            LOG_PRINT(LOG_DEBUG, "File[%s] is Empty.", rsp_path);
            goto err;
        }
        if((buff_ptr = (char *)malloc(img_size)) == NULL)
        {
            LOG_PRINT(LOG_DEBUG, "buff_ptr Malloc Failed!");
            goto err;
        }
        LOG_PRINT(LOG_DEBUG, "img_size = %d", img_size);
        if((rlen = read(fd, buff_ptr, img_size)) == -1)
        {
            LOG_PRINT(LOG_DEBUG, "File[%s] Read Failed.", rsp_path);
            LOG_PRINT(LOG_DEBUG, "Error: %s.", strerror(errno));
            goto err;
        }
        else if(rlen < img_size)
        {
            LOG_PRINT(LOG_DEBUG, "File[%s] Read Not Compeletly.", rsp_path);
            goto err;
        }
        if(img_size < CACHE_MAX_SIZE)
        {
            set_cache_bin(req->thr_arg, cache_key, buff_ptr, img_size);
        }
        goto done;
    }

convert:

    if(got_color == false)
    {
        //compress image
        LOG_PRINT(LOG_DEBUG, "Start to Compress the Image!");
        img_format = MagickGetImageFormat(magick_wand);
        LOG_PRINT(LOG_DEBUG, "Image Format is %s", img_format);
        if(strcmp(img_format, "JPEG") != 0)
        {
            LOG_PRINT(LOG_DEBUG, "Convert Image Format from %s to JPEG.", img_format);
            status = MagickSetImageFormat(magick_wand, "JPEG");
            if(status == MagickFalse)
            {
                LOG_PRINT(LOG_DEBUG, "Image[%s] Convert Format Failed!", orig_path);
            }
            LOG_PRINT(LOG_DEBUG, "Compress Image with JPEGCompression");
            status = MagickSetImageCompression(magick_wand, JPEGCompression);
            if(status == MagickFalse)
            {
                LOG_PRINT(LOG_DEBUG, "Image[%s] Compression Failed!", orig_path);
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
        if(img_size < CACHE_MAX_SIZE)
        {
            set_cache_bin(req->thr_arg, cache_key, buff_ptr, img_size);
        }
        if(settings.save_new == 1 && got_rsp == false)
        {
            LOG_PRINT(LOG_DEBUG, "Color Image[%s] is Not Existed. Begin to Save it.", color_path);
            if(new_img(buff_ptr, img_size, color_path) == -1)
            {
                LOG_PRINT(LOG_DEBUG, "Color Image[%s] Save Failed!", color_path);
                LOG_PRINT(LOG_WARNING, "fail save %s", color_path);
            }
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
            LOG_PRINT(LOG_DEBUG, "Image[%s] Remove Color Failed!", orig_path);
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
        if(img_size < CACHE_MAX_SIZE)
        {
            set_cache_bin(req->thr_arg, cache_key, buff_ptr, img_size);
        }
        if(settings.save_new == 1 && got_rsp == false)
        {
            if(new_img(buff_ptr, img_size, rsp_path) == -1)
            {
                LOG_PRINT(LOG_DEBUG, "New Image[%s] Save Failed!", rsp_path);
                LOG_PRINT(LOG_WARNING, "fail save %s", rsp_path);
            }
        }
        else
            LOG_PRINT(LOG_DEBUG, "Image Needn't to Storage.", rsp_path);
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
    if(fd != -1)
        close(fd);
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


int get_img2(zimg_req_t *req, evhtp_request_t *request)
{
    int result = -1;
    char cache_key[CACHE_KEY_SIZE];
    int fd = -1;
    struct stat f_stat;
    char *buff = NULL;
    struct image *im = NULL;
    size_t len = 0;

    bool is_new = true;

    LOG_PRINT(LOG_DEBUG, "get_img() start processing zimg request...");

    // to gen cache_key like this: 926ee2f570dc50b2575e35a6712b08ce:0:0:1:0
    gen_key(cache_key, req->md5, 3, req->width, req->height, req->proportion);
    if(find_cache_bin(req->thr_arg, cache_key, &buff, &len) == 1)
    {
        LOG_PRINT(LOG_DEBUG, "Hit Cache[Key: %s].", cache_key);
        goto done;
    }
    LOG_PRINT(LOG_DEBUG, "Start to Find the Image...");

    char whole_path[512];
    int lvl1 = str_hash(req->md5);
    int lvl2 = str_hash(req->md5 + 3);
    snprintf(whole_path, 512, "%s/%d/%d/%s", settings.img_path, lvl1, lvl2, req->md5);
    LOG_PRINT(LOG_DEBUG, "docroot: %s", settings.img_path);
    LOG_PRINT(LOG_DEBUG, "req->md5: %s", req->md5);
    LOG_PRINT(LOG_DEBUG, "whole_path: %s", whole_path);

    char name[128];
    if(req->proportion)
        snprintf(name, 128, "%d*%dp", req->width, req->height);
    else
        snprintf(name, 128, "%d*%d", req->width, req->height);

    char orig_path[512];
    snprintf(orig_path, strlen(whole_path) + 6, "%s/0*0", whole_path);
    LOG_PRINT(LOG_DEBUG, "0rig File Path: %s", orig_path);

    char rsp_path[512];
    if(req->width == 0 && req->height == 0 && req->proportion == 0)
    {
        LOG_PRINT(LOG_DEBUG, "Return original image.");
        strncpy(rsp_path, orig_path, 512);
    }
    else
    {
        snprintf(rsp_path, 512, "%s/%s", whole_path, name);
    }
    LOG_PRINT(LOG_DEBUG, "Got the rsp_path: %s", rsp_path);

    if((fd = open(rsp_path, O_RDONLY)) == -1)
    {
        im = wi_new_image();
        if (im == NULL) goto err;
        int ret;
        // to gen cache_key like this: rsp_path-/926ee2f570dc50b2575e35a6712b08ce
        gen_key(cache_key, req->md5, 0);
        if(find_cache_bin(req->thr_arg, cache_key, &buff, &len) == 1)
        {
            LOG_PRINT(LOG_DEBUG, "Hit Orignal Image Cache[Key: %s].", cache_key);

            ret = wi_read_blob(im, buff, len);
            if (ret != WI_OK)
            {
                LOG_PRINT(LOG_DEBUG, "Open Original Image From Blob Failed! Begin to Open it From Disk.");
                del_cache(req->thr_arg, cache_key);
                ret = wi_read_file(im, orig_path);
                if (ret != WI_OK)
                {
                    LOG_PRINT(LOG_DEBUG, "Open Original Image From Disk Failed!");
                    goto err;
                }
                else
                {
                    buff = (char *)wi_get_blob(im, &len);
                    if (buff == NULL) {
                        LOG_PRINT(LOG_DEBUG, "Webimg Get Blob Failed!");
                        goto err;
                    }
                    if(len < CACHE_MAX_SIZE)
                    {
                        set_cache_bin(req->thr_arg, cache_key, (const char *)buff, len);
                    }
                }
            }
        }
        else
        {
            LOG_PRINT(LOG_DEBUG, "Not Hit Original Image Cache. Begin to Open it.");
            ret = wi_read_file(im, orig_path);
            if (ret != WI_OK)
            {
                LOG_PRINT(LOG_DEBUG, "Open Original Image From Disk Failed!");
                goto err;
            }
            else
            {
                buff = (char *)wi_get_blob(im, &len);
                if (buff == NULL) {
                    LOG_PRINT(LOG_DEBUG, "Webimg Get Blob Failed!");
                    goto err;
                }
                if(len < CACHE_MAX_SIZE)
                {
                    set_cache_bin(req->thr_arg, cache_key, (const char *)buff, len);
                }
            }
        }

        ret = convert(im, req);
        if(ret == -1) goto err;
        if(ret == WI_OK) is_new = false;

        buff = (char *)wi_get_blob(im, &len);
        if (buff == NULL) {
            LOG_PRINT(LOG_DEBUG, "Webimg Get Blob Failed!");
            goto err;
        }
    }
    else
    {
        fstat(fd, &f_stat);
        size_t rlen = 0;
        len = f_stat.st_size;
        if(len <= 0)
        {
            LOG_PRINT(LOG_DEBUG, "File[%s] is Empty.", rsp_path);
            goto err;
        }
        if((buff = (char *)malloc(len)) == NULL)
        {
            LOG_PRINT(LOG_DEBUG, "buff Malloc Failed!");
            goto err;
        }
        LOG_PRINT(LOG_DEBUG, "img_size = %d", len);
        if((rlen = read(fd, buff, len)) == -1)
        {
            LOG_PRINT(LOG_DEBUG, "File[%s] Read Failed.", rsp_path);
            LOG_PRINT(LOG_DEBUG, "Error: %s.", strerror(errno));
            goto err;
        }
        else if(rlen < len)
        {
            LOG_PRINT(LOG_DEBUG, "File[%s] Read Not Compeletly.", rsp_path);
            goto err;
        }
    }

    if(settings.save_new == 1 && is_new == false)
    {
        LOG_PRINT(LOG_DEBUG, "Image[%s] is Not Existed. Begin to Save it.", rsp_path);
        if(new_img(buff, len, rsp_path) == -1)
        {
            LOG_PRINT(LOG_DEBUG, "New Image[%s] Save Failed!", rsp_path);
            LOG_PRINT(LOG_WARNING, "fail save %s", rsp_path);
        }
    }
    else
        LOG_PRINT(LOG_DEBUG, "Image [%s] Needn't to Storage.", rsp_path);

    if(len < CACHE_MAX_SIZE)
    {
        gen_key(cache_key, req->md5, 4, req->width, req->height, req->proportion, req->gray);
        set_cache_bin(req->thr_arg, cache_key, buff, len);
    }


done:
    if(settings.etag == 1)
    {
        result = zimg_etag_set(request, buff, len);
        if(result == 2)
            goto err;
    }
    result = evbuffer_add(request->buffer_out, buff, len);
    if(result != -1)
    {
        result = 1;
    }

err:
    if(fd != -1)
        close(fd);
    if(im != NULL)
        wi_free_image(im);
    else if(buff != NULL)
        free(buff);
    return result;
}

