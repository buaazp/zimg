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

extern struct setting settings;

int save_img(const char *buff, const int len, char *md5);
int new_img(const char *buff, const size_t len, const char *save_name);
int get_img(zimg_req_t *req, char **buff_ptr, size_t *img_size);

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
        // to gen cache_key like this: rspPath-/926ee2f570dc50b2575e35a6712b08ce
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
int get_img(zimg_req_t *req, char **buff_ptr, size_t *img_size)
{
    int result = -1;
    char *rspPath = NULL;
    char *cache_key = (char *)malloc(strlen(req->md5) + 32);
    char *whole_path = NULL;
    char *orig_path = NULL;
    char *colorPath = NULL;
    char *img_format = NULL;
    size_t len;
    int fd = -1;
    struct stat f_stat;

    MagickBooleanType status;
    MagickWand *magick_wand = NULL;

    LOG_PRINT(LOG_INFO, "get_img() start processing zimg request...");

    // to gen cache_key like this: img:926ee2f570dc50b2575e35a6712b08ce:0:0:1:0
    sprintf(cache_key, "img:%s:%d:%d:%d:%d", req->md5, req->width, req->height, req->proportion, req->gray);
    if(find_cache_bin(cache_key, buff_ptr, img_size) == 1)
    {
        LOG_PRINT(LOG_INFO, "Hit Cache[Key: %s].", cache_key);
//        sprintf(cache_key, "type:%s:%d:%d:%d:%d", req->md5, req->width, req->height, req->proportion, req->gray);
//        if(find_cache(cache_key, img_type) == -1)
//        {
//            LOG_PRINT(LOG_WARNING, "Cannot Hit Type Cache[Key: %s]. Use jpeg As Default.", cache_key);
//            strcpy(img_type, "jpeg");
//        }
        result = 1;
        goto err;
    }
    LOG_PRINT(LOG_INFO, "Start to Find the Image...");

    len = strlen(req->md5) + strlen(settings.img_path) + 12;
    if (!(whole_path = malloc(len))) {
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

    rspPath = (char *)malloc(512);
    if(req->width == 0 && req->height == 0 && req->gray == 0)
    {
        LOG_PRINT(LOG_INFO, "Return original image.");
        strcpy(rspPath, orig_path);
    }
    else
    {
        sprintf(rspPath, "%s/%s", whole_path, name);
    }
    LOG_PRINT(LOG_INFO, "Got the rspPath: %s", rspPath);
    int got_rsp = 1;


    //status=MagickReadImage(magick_wand, rspPath);
    if((fd = open(rspPath, O_RDONLY)) == -1)
    //if(status == MagickFalse)
    {
        MagickWandGenesis();
        magick_wand = NewMagickWand();
        got_rsp = -1;

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
                    LOG_PRINT(LOG_INFO, "Read Image from Color Image Cache[Key: %s, len: %d] Succ. Goto Convert.", cache_key, *img_size);
                    goto convert;
                }
            }

            len = strlen(rspPath);
            colorPath = (char *)malloc(len);
            strncpy(colorPath, rspPath, len);
            colorPath[len - 1] = '\0';
            LOG_PRINT(LOG_INFO, "colorPath: %s", colorPath);
            status=MagickReadImage(magick_wand, colorPath);
            if(status == MagickTrue)
            {
                LOG_PRINT(LOG_INFO, "Read Image from Color Image[%s] Succ. Goto Convert.", colorPath);
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

        // to gen cache_key like this: rspPath-/926ee2f570dc50b2575e35a6712b08ce
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
        else
        {
            strcpy(rspPath, orig_path);
            got_rsp = 1;
            LOG_PRINT(LOG_INFO, "Args width/height is bigger than real size, return original image.");
        }
    }
    else
    {
        fstat(fd, &f_stat);
        size_t rlen = 0;
        *img_size = f_stat.st_size;
        if((*buff_ptr = (char *)malloc(*img_size)) == NULL)
        {
            LOG_PRINT(LOG_ERROR, "buff_ptr Malloc Failed!");
            goto err;
        }
        LOG_PRINT(LOG_INFO, "img_size = %d", *img_size);
        //*buff_ptr = (char *)MagickGetImageBlob(magick_wand, img_size);
        if(*img_size <= 0)
        {
            LOG_PRINT(LOG_ERROR, "File[%s] is Empty.", rspPath);
            goto err;
        }
        if((rlen = read(fd, *buff_ptr, *img_size)) == -1)
        {
            LOG_PRINT(LOG_ERROR, "File[%s] Read Failed.", rspPath);
            LOG_PRINT(LOG_ERROR, "Error: %s.", strerror(errno));
            goto err;
        }
        else if(rlen < *img_size)
        {
            LOG_PRINT(LOG_ERROR, "File[%s] Read Not Compeletly.", rspPath);
            goto err;
        }
        goto done;
    }



convert:
	//gray image
    if(req->gray == 1)
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

	//compress image
	LOG_PRINT(LOG_INFO, "Start to Compress the Image!");
	const char *img_type = MagickGetFormat(magick_wand);
	if(strcmp(img_type, "JPEG") != 0)
	{
        LOG_PRINT(LOG_INFO, "Conver Image Format from %s to JPEG.", img_type);
		MagickSetImageFormat(magick_wand, "JPEG");
	}
	LOG_PRINT(LOG_INFO, "Compress Image with JPEGCompression");
	MagickSetCompression(magick_wand, JPEGCompression);
	unsigned long quality = MagickGetCompressionQuality(magick_wand) * 0.75;
	if(quality == 0)
	{
		quality = 75;
	}
	LOG_PRINT(LOG_INFO, "Set Compression Quality to 75%.");
	MagickSetCompressionQuality(magick_wand, quality);

	//strip image EXIF infomation
	LOG_PRINT(LOG_INFO, "Start to Remove Exif Infomation of the Image...");
	MagickStripImage(magick_wand);
    *buff_ptr = (char *)MagickGetImageBlob(magick_wand, img_size);
    if(*buff_ptr == NULL)
    {
        LOG_PRINT(LOG_ERROR, "Magick Get Image Blob Failed!");
        goto err;
    }
    goto done;


done:
    if(*img_size < CACHE_MAX_SIZE)
    {
        // to gen cache_key like this: rspPath-/926ee2f570dc50b2575e35a6712b08ce
        sprintf(cache_key, "img:%s:%d:%d:%d:%d", req->md5, req->width, req->height, req->proportion, req->gray);
        set_cache_bin(cache_key, *buff_ptr, *img_size);
//        sprintf(cache_key, "type:%s:%d:%d:%d:%d", req->md5, req->width, req->height, req->proportion, req->gray);
//        set_cache(cache_key, img_type);
    }

    result = 1;
	if(got_rsp == -1)
    {
        LOG_PRINT(LOG_INFO, "Image[%s] is Not Existed. Begin to Save it.", rspPath);
		result = 2;
    }
    else
        LOG_PRINT(LOG_INFO, "Image[%s] is Existed.", rspPath);

err:
    if(fd != -1)
        close(fd);
	req->rspPath = rspPath;
    if(magick_wand)
    {
        magick_wand=DestroyMagickWand(magick_wand);
        MagickWandTerminus();
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

