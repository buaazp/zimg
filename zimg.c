#include "zimg.h"
#include "zmd5.h"

int save_img(const char *buff, const int len, char *md5)
{
    int result = -1;

    LOG_PRINT(LOG_INFO, "Begin to Caculate MD5...");
    md5_state_t mdctx;
    md5_byte_t md_value[16];
    char md5sum[33];
    unsigned int md_len, i;
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

    char *savePath = (char *)malloc(512);
    char *saveName = (char *)malloc(512);
    sprintf(savePath, "%s/%s", _img_path, md5sum);
    LOG_PRINT(LOG_INFO, "savePath: %s", savePath);
    if(is_dir(savePath) == -1)
    {
        if(mk_dir(savePath) == -1)
        {
            LOG_PRINT(LOG_ERROR, "savePath[%s] Create Failed!", savePath);
            goto err;
        }
    }

    sprintf(saveName, "%s/0*0p", savePath);
    LOG_PRINT(LOG_INFO, "saveName-->: %s", saveName);
	if(new_img(buff, len, saveName) == -1)
	{
		LOG_PRINT(LOG_WARNING, "Save Image[%s] Failed!", saveName);
	}
	result = 1;

err:
    if(savePath)
        free(savePath);
    if(saveName)
        free(saveName);
    return result;
}

int new_img(const char *buff, const size_t len, const char *saveName)
{
	int result = -1;
	LOG_PRINT(LOG_INFO, "Start to Storage the New Image...");
	int fd = -1;
	int wlen = 0;

	if((fd = open(saveName, O_WRONLY|O_TRUNC|O_CREAT, 00644)) < 0)
	{
		LOG_PRINT(LOG_ERROR, "fd open failed!");
		goto err;
	}
	wlen = write(fd, buff, len);
	if(wlen == -1)
	{
		LOG_PRINT(LOG_ERROR, "write() failed!");
		goto err;
	}
	else if(wlen < len)
	{
		LOG_PRINT(LOG_ERROR, "Only part of data is been writed.");
		goto err;
	}
	LOG_PRINT(LOG_INFO, "Image [%s] Write Successfully!", saveName);
	result = 1;

err:
	if(fd != -1)
		close(fd);
	return result;
}

/* get image method used for zimg servise, such as:
 * http://127.0.0.1:4869/c6c4949e54afdb0972d323028657a1ef?w=100&h=50&p=1&g=1 */
int get_img(zimg_req_t *req, char **buff, char *img_type, size_t *img_size)
{
    int result = -1;
    char *rspPath = (char *)malloc(512);
    char *whole_path = NULL;
    char *origPath = NULL;
    size_t len;
    MagickBooleanType status;
    MagickWand *magick_wand;
    MagickWandGenesis();
    magick_wand = NewMagickWand();

    LOG_PRINT(LOG_INFO, "get_img() start processing zimg request...");

    len = strlen(req->md5) + strlen(_img_path) + 2;
    if (!(whole_path = malloc(len))) {
        LOG_PRINT(LOG_ERROR, "whole_path malloc failed!");
        goto err;
    }
    evutil_snprintf(whole_path, len, "%s/%s", _img_path, req->md5);
    LOG_PRINT(LOG_INFO, "docroot: %s", _img_path);
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

    origPath = (char *)malloc(strlen(whole_path) + 6);
    sprintf(origPath, "%s/0*0p", whole_path);
    LOG_PRINT(LOG_INFO, "0rig File Path: %s", origPath);

    if(req->width == 0 && req->height == 0)
    {
        LOG_PRINT(LOG_INFO, "Return original image.");
        strcpy(rspPath, origPath);
    }
    else
    {
        sprintf(rspPath, "%s/%s", whole_path, name);
    }
    LOG_PRINT(LOG_INFO, "Got the rspPath: %s", rspPath);

    LOG_PRINT(LOG_INFO, "Start to Find the Image...");
    int got_rsp = 1;
    status=MagickReadImage(magick_wand, rspPath);
    if(status == MagickFalse)
    {
        got_rsp = -1;
        status = MagickReadImage(magick_wand, origPath);
        if(status == MagickFalse)
        {
            ThrowWandException(magick_wand);
            goto err;
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
                else if(height != 0 && width == 0)
                {
                    width = height * owidth / oheight;
                }
            }
            status = MagickResizeImage(magick_wand, width, height, LanczosFilter, 1.0);
            if(status == MagickFalse)
            {
                LOG_PRINT(LOG_ERROR, "Image[%s] Resize Failed!", origPath);
                goto err;
            }
            LOG_PRINT(LOG_INFO, "Resize img succ.");
        }
        else
        {
            strcpy(rspPath, origPath);
            got_rsp = 1;
            LOG_PRINT(LOG_INFO, "Args width/height is bigger than real size, return original image.");
        }
    }
    char *img_format = NULL;
    img_format = MagickGetImageFormat(magick_wand);
    strcpy(img_type, img_format);
    if(img_format)
        free(img_format);
    LOG_PRINT(LOG_INFO, "Got Image Format: %s", img_type);

    *buff = MagickGetImageBlob(magick_wand, img_size);
	req->rspPath = rspPath;

    result = 1;
	if(got_rsp == -1)
		result = 2;
    else
        LOG_PRINT(LOG_INFO, "Image[%s] is Existed.", rspPath);
err:
    magick_wand=DestroyMagickWand(magick_wand);
    MagickWandTerminus();
    if (origPath)
        free(origPath);
    if (whole_path)
        free(whole_path);
    return result;
}

