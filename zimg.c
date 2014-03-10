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

#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <wand/MagickWand.h>
#include "zimg.h"
#include "zmd5.h"
#include "zlog.h"
#include "zcache.h"
#include "zutil.h"
#include <hiredis/hiredis.h>

extern struct setting settings;

int save_img(const char *buff, const int len, char *md5);
int new_img(const char *buff, const size_t len, const char *save_name);
int get_img(zimg_req_t *req, char **buff_ptr, size_t *img_size);
int get_img2(zimg_req_t *req, char **buff_ptr, size_t *img_size);
int save_img_ssdb(const char *cache_key, const char *buff, const size_t len);
int get_img_ssdb(const char *cache_key, char **buff, size_t *len);

/**
 * @brief save_img Save buffer from POST requests
 *
 * @param buff The char * from POST request
 * @param len The length of buff
 * @param md5 Parsed md5 from url
 *
 * @return 1 for success and -1 for fail
 */
int save_img(const char *buff, const int len, char *md5)
{
    int result = -1;

    LOG_PRINT(LOG_INFO, "Begin to Caculate MD5...");
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
    LOG_PRINT(LOG_INFO, "md5: %s", md5sum);

    char *cache_key = (char *)malloc(strlen(md5sum) + 32);
    char *save_path = (char *)malloc(512);
    char *save_name = (char *)malloc(512);

    sprintf(cache_key, "img:%s:0:0:1:0", md5sum);
    if(exist_cache(cache_key) == 1)
    {
        LOG_PRINT(LOG_INFO, "File Exist, Needn't Save.");
        result = 1;
        goto done;
    }
    LOG_PRINT(LOG_INFO, "exist_cache not found. Begin to Check File.");

    if(settings.mode == 2)
    {
        if(save_img_ssdb(cache_key, buff, len) == -1)
        {
            goto done;
        }
        else
        {
            result = 1;
            goto done;
        }
    }

    //caculate 2-level path
    int lvl1 = str_hash(md5sum);
    int lvl2 = str_hash(md5sum + 3);

    sprintf(save_path, "%s/%d/%d/%s", settings.img_path, lvl1, lvl2, md5sum);
    LOG_PRINT(LOG_INFO, "save_path: %s", save_path);

    if(is_dir(save_path) == 1)
    {
        LOG_PRINT(LOG_INFO, "Check File Exist. Needn't Save.");
        goto cache;
    }

    if(mk_dirs(save_path) == -1)
    {
        LOG_PRINT(LOG_ERROR, "save_path[%s] Create Failed!", save_path);
        goto done;
    }
    chdir(_init_path);
    LOG_PRINT(LOG_INFO, "save_path[%s] Create Finish.", save_path);
    sprintf(save_name, "%s/0*0p", save_path);
    LOG_PRINT(LOG_INFO, "save_name-->: %s", save_name);
    if(new_img(buff, len, save_name) == -1)
    {
        LOG_PRINT(LOG_WARNING, "Save Image[%s] Failed!", save_name);
        goto done;
    }

cache:
    if(len < CACHE_MAX_SIZE)
    {
        // to gen cache_key like this: rsp_path-/926ee2f570dc50b2575e35a6712b08ce
        sprintf(cache_key, "img:%s:0:0:1:0", md5sum);
        set_cache_bin(cache_key, buff, len);
//        sprintf(cache_key, "type:%s:0:0:1:0", md5sum);
//        set_cache(cache_key, type);
    }
    result = 1;

done:
    if(cache_key)
        free(cache_key);
    if(save_path)
        free(save_path);
    if(save_name)
        free(save_name);
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
    LOG_PRINT(LOG_INFO, "Start to Storage the New Image...");
    int fd = -1;
    int wlen = 0;

    if((fd = open(save_name, O_WRONLY | O_TRUNC | O_CREAT, 00644)) < 0)
    {
        LOG_PRINT(LOG_ERROR, "fd(%s) open failed!", save_name);
        goto done;
    }

    if(flock(fd, LOCK_EX | LOCK_NB) == -1)
    {
        LOG_PRINT(LOG_WARNING, "This fd is Locked by Other thread.");
        goto done;
    }

    wlen = write(fd, buff, len);
    if(wlen == -1)
    {
        LOG_PRINT(LOG_ERROR, "write(%s) failed!", save_name);
        goto done;
    }
    else if(wlen < len)
    {
        LOG_PRINT(LOG_ERROR, "Only part of [%s] is been writed.", save_name);
        goto done;
    }
    flock(fd, LOCK_UN | LOCK_NB);
    LOG_PRINT(LOG_INFO, "Image [%s] Write Successfully!", save_name);
    result = 1;

done:
    if(fd != -1)
    {
        close(fd);
    }
    return result;
}

/* get image method used for zimg servise, such as:
 * http://127.0.0.1:4869/c6c4949e54afdb0972d323028657a1ef?w=100&h=50&p=1&g=1 */
/**
 * @brief get_img The function of getting a image buffer and it's length.
 *
 * @param req The zimg_req_t from zhttp and it has the params of a request.
 * @param buff_ptr This function return image buffer in it.
 * @param img_size Get_img will change this number to return the size of image buffer.
 *
 * @return 1 for success and -1 for fail.
 */
int get_img(zimg_req_t *req, char **buff_ptr, size_t *img_size)
{
    int result = -1;
    char *rsp_path = NULL;
    char *cache_key = (char *)malloc(strlen(req->md5) + 32);
    char *whole_path = NULL;
    char *orig_path = NULL;
    char *color_path = NULL;
    char *img_format = NULL;
    size_t len;
    int fd = -1;
    struct stat f_stat;

    MagickBooleanType status;
    MagickWand *magick_wand = NULL;

    bool got_rsp = true;
    bool got_color = false;

    LOG_PRINT(LOG_INFO, "get_img() start processing zimg request...");

    // to gen cache_key like this: img:926ee2f570dc50b2575e35a6712b08ce:0:0:1:0
    sprintf(cache_key, "img:%s:%d:%d:%d:%d", req->md5, req->width, req->height, req->proportion, req->gray);
    if(find_cache_bin(cache_key, buff_ptr, img_size) == 1)
    {
        LOG_PRINT(LOG_INFO, "Hit Cache[Key: %s].", cache_key);
//        sprintf(cache_key, "type:%s:%d:%d:%d:%d", req->md5, req->width, req->height, req->proportion, req->gray);
//        if(find_cache(cache_key, img_format) == -1)
//        {
//            LOG_PRINT(LOG_WARNING, "Cannot Hit Type Cache[Key: %s]. Use jpeg As Default.", cache_key);
//            strcpy(img_format, "jpeg");
//        }
        result = 1;
        goto err;
    }
    LOG_PRINT(LOG_INFO, "Start to Find the Image...");

    len = strlen(req->md5) + strlen(settings.img_path) + 12;
    if (!(whole_path = malloc(len)))
    {
        LOG_PRINT(LOG_ERROR, "whole_path malloc failed!");
        goto err;
    }
    int lvl1 = str_hash(req->md5);
    int lvl2 = str_hash(req->md5 + 3);
    sprintf(whole_path, "%s/%d/%d/%s", settings.img_path, lvl1, lvl2, req->md5);
    LOG_PRINT(LOG_INFO, "docroot: %s", settings.img_path);
    LOG_PRINT(LOG_INFO, "req->md5: %s", req->md5);
    LOG_PRINT(LOG_INFO, "whole_path: %s", whole_path);

    char name[128];
    if(req->proportion && req->gray)
        sprintf(name, "%d*%dpg", req->width, req->height);
    else if(req->proportion && !req->gray)
        sprintf(name, "%d*%dp", req->width, req->height);
    else if(!req->proportion && req->gray)
        sprintf(name, "%d*%dg", req->width, req->height);
    else
        sprintf(name, "%d*%d", req->width, req->height);

    orig_path = (char *)malloc(strlen(whole_path) + 6);
    sprintf(orig_path, "%s/0*0p", whole_path);
    LOG_PRINT(LOG_INFO, "0rig File Path: %s", orig_path);

    rsp_path = (char *)malloc(512);
    if(req->width == 0 && req->height == 0 && req->gray == 0)
    {
        LOG_PRINT(LOG_INFO, "Return original image.");
        strcpy(rsp_path, orig_path);
    }
    else
    {
        sprintf(rsp_path, "%s/%s", whole_path, name);
    }
    LOG_PRINT(LOG_INFO, "Got the rsp_path: %s", rsp_path);


    //status=MagickReadImage(magick_wand, rsp_path);
    if((fd = open(rsp_path, O_RDONLY)) == -1)
        //if(status == MagickFalse)
    {
        magick_wand = NewMagickWand();
        got_rsp = false;

        if(req->gray == 1)
        {
            sprintf(cache_key, "img:%s:%d:%d:%d:0", req->md5, req->width, req->height, req->proportion);
            if(find_cache_bin(cache_key, buff_ptr, img_size) == 1)
            {
                LOG_PRINT(LOG_INFO, "Hit Color Image Cache[Key: %s, len: %d].", cache_key, *img_size);
                status = MagickReadImageBlob(magick_wand, *buff_ptr, *img_size);
                if(status == MagickFalse)
                {
                    LOG_PRINT(LOG_WARNING, "Color Image Cache[Key: %s] is Bad. Remove.", cache_key);
                    del_cache(cache_key);
                }
                else
                {
                    got_color = true;
                    LOG_PRINT(LOG_INFO, "Read Image from Color Image Cache[Key: %s, len: %d] Succ. Goto Convert.", cache_key, *img_size);
                    goto convert;
                }
            }

            len = strlen(rsp_path);
            color_path = (char *)malloc(len);
            strncpy(color_path, rsp_path, len);
            color_path[len - 1] = '\0';
            LOG_PRINT(LOG_INFO, "color_path: %s", color_path);
            status=MagickReadImage(magick_wand, color_path);
            if(status == MagickTrue)
            {
                got_color = true;
                LOG_PRINT(LOG_INFO, "Read Image from Color Image[%s] Succ. Goto Convert.", color_path);
                *buff_ptr = (char *)MagickGetImageBlob(magick_wand, img_size);
                if(*img_size < CACHE_MAX_SIZE)
                {
                    set_cache_bin(cache_key, *buff_ptr, *img_size);
//                    img_format = MagickGetImageFormat(magick_wand);
//                    sprintf(cache_key, "type:%s:%d:%d:%d:0", req->md5, req->width, req->height, req->proportion);
//                    set_cache(cache_key, img_format);
                }

                goto convert;
            }
        }

        // to gen cache_key like this: rsp_path-/926ee2f570dc50b2575e35a6712b08ce
        sprintf(cache_key, "img:%s:0:0:1:0", req->md5);
        if(find_cache_bin(cache_key, buff_ptr, img_size) == 1)
        {
            LOG_PRINT(LOG_INFO, "Hit Orignal Image Cache[Key: %s].", cache_key);
            status = MagickReadImageBlob(magick_wand, *buff_ptr, *img_size);
            if(status == MagickFalse)
            {
                LOG_PRINT(LOG_WARNING, "Open Original Image From Blob Failed! Begin to Open it From Disk.");
                ThrowWandException(magick_wand);
                del_cache(cache_key);
                status = MagickReadImage(magick_wand, orig_path);
                if(status == MagickFalse)
                {
                    ThrowWandException(magick_wand);
                    goto err;
                }
                else
                {
                    *buff_ptr = (char *)MagickGetImageBlob(magick_wand, img_size);
                    if(*img_size < CACHE_MAX_SIZE)
                    {
                        set_cache_bin(cache_key, *buff_ptr, *img_size);
//                        img_format = MagickGetImageFormat(magick_wand);
//                        sprintf(cache_key, "type:%s:0:0:1:0", req->md5);
//                        set_cache(cache_key, img_format);
                    }
                }
            }
        }
        else
        {
            LOG_PRINT(LOG_INFO, "Not Hit Original Image Cache. Begin to Open it.");
            status = MagickReadImage(magick_wand, orig_path);
            if(status == MagickFalse)
            {
                ThrowWandException(magick_wand);
                goto err;
            }
            else
            {
                *buff_ptr = (char *)MagickGetImageBlob(magick_wand, img_size);
                if(*img_size < CACHE_MAX_SIZE)
                {
                    set_cache_bin(cache_key, *buff_ptr, *img_size);
//                    img_format = MagickGetImageFormat(magick_wand);
//                    sprintf(cache_key, "type:%s:0:0:1:0", req->md5);
//                    set_cache(cache_key, img_format);
                }
            }
        }

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
                //else if(height != 0 && width == 0)
                else
                {
                    width = height * owidth / oheight;
                }
            }
            status = MagickResizeImage(magick_wand, width, height, LanczosFilter, 1.0);
            if(status == MagickFalse)
            {
                LOG_PRINT(LOG_ERROR, "Image[%s] Resize Failed!", orig_path);
                goto err;
            }
            LOG_PRINT(LOG_INFO, "Resize img succ.");
        }
        /* this section can caculate the correct rsp_path, but not use, so I note them
           else if(req->gray == true)
           {
           strcpy(rsp_path, orig_path);
           rsp_path[strlen(orig_path)] = 'g';
           rsp_path[strlen(orig_path) + 1] = '\0';
           got_rsp = true;
           LOG_PRINT(LOG_INFO, "Args width/height is bigger than real size, return original gray image.");
           LOG_PRINT(LOG_INFO, "Original Gray Image: %s", rsp_path);
           }
           */
        else
        {
            // Note this strcpy because rsp_path is not useful. We needn't to save the new image.
            //strcpy(rsp_path, orig_path);
            got_rsp = true;
            LOG_PRINT(LOG_INFO, "Args width/height is bigger than real size, return original image.");
        }
    }
    else
    {
        fstat(fd, &f_stat);
        size_t rlen = 0;
        *img_size = f_stat.st_size;
        if(*img_size <= 0)
        {
            LOG_PRINT(LOG_ERROR, "File[%s] is Empty.", rsp_path);
            goto err;
        }
        if((*buff_ptr = (char *)malloc(*img_size)) == NULL)
        {
            LOG_PRINT(LOG_ERROR, "buff_ptr Malloc Failed!");
            goto err;
        }
        LOG_PRINT(LOG_INFO, "img_size = %d", *img_size);
        //*buff_ptr = (char *)MagickGetImageBlob(magick_wand, img_size);
        if((rlen = read(fd, *buff_ptr, *img_size)) == -1)
        {
            LOG_PRINT(LOG_ERROR, "File[%s] Read Failed.", rsp_path);
            LOG_PRINT(LOG_ERROR, "Error: %s.", strerror(errno));
            goto err;
        }
        else if(rlen < *img_size)
        {
            LOG_PRINT(LOG_ERROR, "File[%s] Read Not Compeletly.", rsp_path);
            goto err;
        }
        goto done;
    }



convert:
    //gray image
    if(req->gray == true)
    {
        LOG_PRINT(LOG_INFO, "Start to Remove Color!");
        status = MagickSetImageColorspace(magick_wand, GRAYColorspace);
        if(status == MagickFalse)
        {
            LOG_PRINT(LOG_ERROR, "Image[%s] Remove Color Failed!", orig_path);
            goto err;
        }
        LOG_PRINT(LOG_INFO, "Image Remove Color Finish!");
    }

    if(got_color == false || (got_color == true && req->width == 0) )
    {
        //compress image
        LOG_PRINT(LOG_INFO, "Start to Compress the Image!");
        img_format = MagickGetImageFormat(magick_wand);
        LOG_PRINT(LOG_INFO, "Image Format is %s", img_format);
        if(strcmp(img_format, "JPEG") != 0)
        {
            LOG_PRINT(LOG_INFO, "Convert Image Format from %s to JPEG.", img_format);
            status = MagickSetImageFormat(magick_wand, "JPEG");
            if(status == MagickFalse)
            {
                LOG_PRINT(LOG_WARNING, "Image[%s] Convert Format Failed!", orig_path);
            }
            LOG_PRINT(LOG_INFO, "Compress Image with JPEGCompression");
            status = MagickSetImageCompression(magick_wand, JPEGCompression);
            if(status == MagickFalse)
            {
                LOG_PRINT(LOG_WARNING, "Image[%s] Compression Failed!", orig_path);
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
    }
    *buff_ptr = (char *)MagickGetImageBlob(magick_wand, img_size);
    if(*buff_ptr == NULL)
    {
        LOG_PRINT(LOG_ERROR, "Magick Get Image Blob Failed!");
        goto err;
    }


done:
    if(*img_size < CACHE_MAX_SIZE)
    {
        // to gen cache_key like this: rsp_path-/926ee2f570dc50b2575e35a6712b08ce
        sprintf(cache_key, "img:%s:%d:%d:%d:%d", req->md5, req->width, req->height, req->proportion, req->gray);
        set_cache_bin(cache_key, *buff_ptr, *img_size);
//        sprintf(cache_key, "type:%s:%d:%d:%d:%d", req->md5, req->width, req->height, req->proportion, req->gray);
//        set_cache(cache_key, img_format);
    }

    result = 1;
    if(got_rsp == false)
    {
        LOG_PRINT(LOG_INFO, "Image[%s] is Not Existed. Begin to Save it.", rsp_path);
        result = 2;
    }
    else
        LOG_PRINT(LOG_INFO, "Image Needn't to Storage.", rsp_path);

err:
    if(fd != -1)
        close(fd);
    req->rsp_path = rsp_path;
    if(magick_wand)
    {
        magick_wand=DestroyMagickWand(magick_wand);
    }
    if(img_format)
        free(img_format);
    if(cache_key)
        free(cache_key);
    if (orig_path)
        free(orig_path);
    if (whole_path)
        free(whole_path);
    return result;
}


int get_img2(zimg_req_t *req, char **buff_ptr, size_t *img_size)
{
    int result = -1;
    char *cache_key = (char *)malloc(strlen(req->md5) + 32);
    char *img_format = NULL;
    size_t len;

    MagickBooleanType status;
    MagickWand *magick_wand = NULL;

    bool got_color = false;

    LOG_PRINT(LOG_INFO, "get_img() start processing zimg request...");

    sprintf(cache_key, "img:%s:%d:%d:%d:%d", req->md5, req->width, req->height, req->proportion, req->gray);
    if(find_cache_bin(cache_key, buff_ptr, img_size) == 1)
    {
        LOG_PRINT(LOG_INFO, "Hit Cache[Key: %s].", cache_key);
        result = 1;
        goto err;
    }
    LOG_PRINT(LOG_INFO, "Start to Find the Image...");
    if(get_img_ssdb(cache_key, buff_ptr, img_size) == 1)
    {
        LOG_PRINT(LOG_INFO, "Get image [%s] from ssdb succ.", cache_key);
        result = 1;
        goto err;
    }
    if(req->width == 0 && req->height == 0 && req->gray == 0)
    {
        sprintf(cache_key, "img:%s:0:0:1:0", req->md5);
        LOG_PRINT(LOG_INFO, "Return original image.");
        if(find_cache_bin(cache_key, buff_ptr, img_size) == 1)
        {
            LOG_PRINT(LOG_INFO, "Hit Cache[Key: %s].", cache_key);
            result = 1;
            goto err;
        }
        if(get_img_ssdb(cache_key, buff_ptr, img_size) == 1)
        {
            LOG_PRINT(LOG_INFO, "Get color image [%s] from ssdb.", cache_key);
            result = 1;
            goto err;
        }
        else
        {
            LOG_PRINT(LOG_ERROR, "Get image [%s] from ssdb failed.", cache_key);
            goto err;
        }
    }

    magick_wand = NewMagickWand();
    if(req->gray == 1)
    {
        sprintf(cache_key, "img:%s:%d:%d:%d:0", req->md5, req->width, req->height, req->proportion);
        if(find_cache_bin(cache_key, buff_ptr, img_size) == 1)
        {
            LOG_PRINT(LOG_INFO, "Hit Color Image Cache[Key: %s, len: %d].", cache_key, *img_size);
            status = MagickReadImageBlob(magick_wand, *buff_ptr, *img_size);
            if(status == MagickFalse)
            {
                LOG_PRINT(LOG_WARNING, "Color Image Cache[Key: %s] is Bad. Remove.", cache_key);
                del_cache(cache_key);
            }
            else
            {
                got_color = true;
                LOG_PRINT(LOG_INFO, "Read Image from Color Image Cache[Key: %s, len: %d] Succ. Goto Convert.", cache_key, *img_size);
                goto convert;
            }
        }
        if(get_img_ssdb(cache_key, buff_ptr, img_size) == 1)
        {
            LOG_PRINT(LOG_INFO, "Get color image [%s] from ssdb.", cache_key);
            status = MagickReadImageBlob(magick_wand, *buff_ptr, *img_size);
            if(status == MagickTrue)
            {
                got_color = true;
                LOG_PRINT(LOG_INFO, "Read Image from Color Image[%s] Succ. Goto Convert.", cache_key);
                *buff_ptr = (char *)MagickGetImageBlob(magick_wand, img_size);
                if(*img_size < CACHE_MAX_SIZE)
                {
                    set_cache_bin(cache_key, *buff_ptr, *img_size);
                }
                goto convert;
            }
        }
    }

    sprintf(cache_key, "img:%s:0:0:1:0", req->md5);
    if(find_cache_bin(cache_key, buff_ptr, img_size) == 1)
    {
        LOG_PRINT(LOG_INFO, "Hit Cache[Key: %s].", cache_key);
    }
    else if(get_img_ssdb(cache_key, buff_ptr, img_size) == -1)
    {
        LOG_PRINT(LOG_ERROR, "Get image [%s] from ssdb failed.", cache_key);
        goto err;
    }
    else
    {
        set_cache_bin(cache_key, *buff_ptr, *img_size);
    }

    status = MagickReadImageBlob(magick_wand, *buff_ptr, *img_size);
    if(status == MagickFalse)
    {
        ThrowWandException(magick_wand);
        del_cache(cache_key);
        LOG_PRINT(LOG_ERROR, "Get image [%s] from ssdb failed.", cache_key);
        goto err;
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
                goto err;
            }
            LOG_PRINT(LOG_INFO, "Resize img succ.");
        }
        else
        {
            LOG_PRINT(LOG_INFO, "Args width/height is bigger than real size, return original image.");
            result = 1;
            goto err;
        }
    }


convert:
    //gray image
    if(req->gray == true)
    {
        LOG_PRINT(LOG_INFO, "Start to Remove Color!");
        status = MagickSetImageColorspace(magick_wand, GRAYColorspace);
        if(status == MagickFalse)
        {
            LOG_PRINT(LOG_ERROR, "Image[%s] Remove Color Failed!", cache_key);
            goto err;
        }
        LOG_PRINT(LOG_INFO, "Image Remove Color Finish!");
    }

    if(got_color == false || (got_color == true && req->width == 0) )
    {
        //compress image
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
    }
    *buff_ptr = (char *)MagickGetImageBlob(magick_wand, img_size);
    if(*buff_ptr == NULL)
    {
        LOG_PRINT(LOG_ERROR, "Magick Get Image Blob Failed!");
        goto err;
    }


done:
    sprintf(cache_key, "img:%s:%d:%d:%d:%d", req->md5, req->width, req->height, req->proportion, req->gray);
    save_img_ssdb(cache_key, *buff_ptr, *img_size);
    if(*img_size < CACHE_MAX_SIZE)
    {
        set_cache_bin(cache_key, *buff_ptr, *img_size);
    }
    result = 1;

err:
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
