#include "zimg.h"
#include "zmd5.h"

int save_img(const char *type, const char *buff, const int len, char *md5)
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

//    char *md5sum = "5f189d8ec57f5a5a0d3dcba47fa797e2";

    int fd = -1;
    int wlen = 0;

    char *savePath = (char *)malloc(512);
    char *saveName = (char *)malloc(512);
    char *origName = (char *)malloc(256);
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

    sprintf(origName, "0*0p.%s", type);
    sprintf(saveName, "%s/%s", savePath, origName);
    LOG_PRINT(LOG_INFO, "saveName-->: %s", saveName);

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

    // to gen cacheKey like this: rspPath-/926ee2f570dc50b2575e35a6712b08ce
    char *cacheKey = (char *)malloc(strlen(md5sum) + 10);
    sprintf(cacheKey, "path:%s:0:0:1:0", md5sum);
    set_cache(cacheKey, saveName);
    free(cacheKey);

err:
    if(fd != -1)
        close(fd);
    if(savePath)
        free(savePath);
    if(saveName)
        free(saveName);
    if(origName)
        free(origName);
    return result;
}


/* get image method used for zimg servise, such as:
 * http://127.0.0.1:4869/c6c4949e54afdb0972d323028657a1ef?w=100&h=50&p=1&g=1 */
int get_img(zimg_req_t *req, struct evbuffer *evb, char *imgType)
{
    int result = -1;
    char rspPath[512];
    char *whole_path = NULL;
    size_t len;
    struct stat st;
    int fd = -1;
    int isGenRsp = -1;
    char cacheKey[256];
    DIR *dir = NULL;

    LOG_PRINT(LOG_INFO, "get_img() start processing zimg request...");

    sprintf(cacheKey, "path:%s:%d:%d:%d:%d", req->md5, req->width, req->height, req->proportion, req->gray);
    if(find_cache(cacheKey, rspPath) == 1)
    {
        LOG_PRINT(LOG_INFO, "Hit Cache. rspPath: %s", rspPath);
        goto openFile;
    }

genRspPath:
    len = strlen(req->md5) + strlen(_img_path) + 2;
    if (!(whole_path = malloc(len))) {
        LOG_PRINT(LOG_ERROR, "malloc failed!");
        goto err;
    }
    evutil_snprintf(whole_path, len, "%s/%s", _img_path, req->md5);
    LOG_PRINT(LOG_INFO, "docroot: %s", _img_path);
    LOG_PRINT(LOG_INFO, "req->md5: %s", req->md5);
    LOG_PRINT(LOG_INFO, "whole_path: %s", whole_path);

    if (stat(whole_path, &st)<0) {
        LOG_PRINT(LOG_ERROR, "stat whole_path[%s] failed!", whole_path);
        goto err;
    }

    if (!S_ISDIR(st.st_mode)) 
    {
        LOG_PRINT(LOG_ERROR, "MD5[%s] not find.", req->md5);
        goto err;
    }
    /* If it's a directory, read the comments and make a little
     * index page */
    char origPath[512];
    char name[128];
    if(req->proportion && req->gray)
        sprintf(name, "%d*%dpg", req->width, req->height);
    else if(req->proportion && !req->gray)
        sprintf(name, "%d*%dp", req->width, req->height);
    else if(!req->proportion && req->gray)
        sprintf(name, "%d*%dg", req->width, req->height);
    else
        sprintf(name, "%d*%d", req->width, req->height);

    sprintf(cacheKey, "path:%s:0:0:1:0", req->md5);
    if(find_cache(cacheKey, origPath) == 1)
    {
        LOG_PRINT(LOG_INFO, "Hit Cache. origPath: %s", origPath);
        goto getOrigPath;
    }
    if (!(dir = opendir(whole_path)))
    {
        LOG_PRINT(LOG_ERROR, "Dir[%s] open failed.", whole_path);
        goto err;
    }
    struct dirent *ent = NULL;
    char *origName = NULL;
    int find = 0;

    while(ent = readdir(dir))
    {
        const char *tmpName = ent->d_name;
        if(strstr(tmpName, "0*0") == tmpName) // name "0*0" to find it first
        {
            len = strlen(tmpName) + 1;
            if(!(origName = malloc(len)))
            {
                LOG_PRINT(LOG_ERROR, "malloc");
                goto err;
            }
            strcpy(origName, tmpName);
            find = 1;
            break;
        }
    }
    if(find == 0)
    {
        LOG_PRINT(LOG_ERROR, "Get 0rig Image Failed.");
        goto err;
    }

    LOG_PRINT(LOG_INFO, "0rigName: %s", origName);
    sprintf(origPath, "%s/%s", whole_path, origName);
    LOG_PRINT(LOG_INFO, "0rig File Path: %s", origPath);

    sprintf(cacheKey, "path:%s:0:0:1:0", req->md5);
    set_cache(cacheKey, origPath);

getOrigPath:
    LOG_PRINT(LOG_INFO, "Goto getOrigPath...");
    if(req->width == 0 && req->height == 0)
    {
        LOG_PRINT(LOG_INFO, "Return original image.");
        strcpy(rspPath, origPath);
    }
    else
    {
        // rspPath = whole_path / 1024*768 . jpeg \0
        //char imgType[32];
        get_type(origPath, imgType);
        len = strlen(whole_path) + strlen(name) + strlen(imgType) + 3; 
        if(strlen(imgType) == 0)
            sprintf(rspPath, "%s/%s", whole_path, name);
        else
            sprintf(rspPath, "%s/%s.%s", whole_path, name, imgType);
    }
    LOG_PRINT(LOG_INFO, "File Path: %s", rspPath);

    isGenRsp = 1;
    LOG_PRINT(LOG_INFO, "Section genRspPath has steped in. isGenRsp = %d", isGenRsp);
    sprintf(cacheKey, "path:%s:%d:%d:%d:%d", req->md5, req->width, req->height, req->proportion, req->gray);
    LOG_PRINT(LOG_INFO, "cacheKey = %s", cacheKey);
    set_cache(cacheKey, rspPath);

openFile:
    LOG_PRINT(LOG_INFO, "Goto Section openFile.");
    if ((fd = open(rspPath, O_RDONLY)) < 0) 
    {
        if(isGenRsp == -1)
        {
            LOG_PRINT(LOG_INFO, "Section genRspPath haven't step in. isGenRsp = %d. Goto genRspPath...", isGenRsp);
            goto genRspPath;
        }
        LOG_PRINT(LOG_INFO, "Not find the file, begin to resize...");
        MagickBooleanType status;
        MagickWand *magick_wand;
        MagickWandGenesis();
        magick_wand=NewMagickWand();
        status=MagickReadImage(magick_wand, origPath);
        if (status == MagickFalse)
        {
            ThrowWandException(magick_wand);
            goto err;
        }

        int width, height;
        width = req->width;
        height = req->height;
        if(req->proportion == 1)
        {
            if(req->width != 0 && req->height == 0)
            {
                float owidth = MagickGetImageWidth(magick_wand);
                float oheight = MagickGetImageHeight(magick_wand);
                height = width * oheight / owidth;
            }
            else if(height != 0 && width == 0)
            {
                int oheight = MagickGetImageHeight(magick_wand);
                float owidth = MagickGetImageWidth(magick_wand);
                width = height * owidth / oheight;
            }
        }
        MagickResetIterator(magick_wand);
        while (MagickNextImage(magick_wand) != MagickFalse)
            MagickResizeImage(magick_wand, width, height, LanczosFilter, 1.0);
        LOG_PRINT(LOG_INFO, "Resize img succ.");
        MagickSizeType imgSize;
        status = MagickGetImageLength(magick_wand, &imgSize);
        if (status == MagickFalse)
        {
            ThrowWandException(magick_wand);
            goto err;
        }
        size_t imgSizet = imgSize;
        char *imgBuff = MagickGetImageBlob(magick_wand, &imgSizet);
        //Designed for High Performance: Reply Customer First, Storage Image Second.
        evbuffer_add(evb, imgBuff, imgSizet);
        status=MagickWriteImages(magick_wand, rspPath, MagickTrue);
        if (status == MagickFalse)
        {
            ThrowWandException(magick_wand);
            sprintf(cacheKey, "path:%s:%d:%d:%d:%d", req->md5, req->width, req->height, req->proportion, req->gray);
            del_cache(cacheKey);
            LOG_PRINT(LOG_WARNING, "New img[%s] Write Failed!", rspPath);
        }
        else
        {
            LOG_PRINT(LOG_INFO, "New img[%s] storaged.", rspPath);
        }
        magick_wand=DestroyMagickWand(magick_wand);
        MagickWandTerminus();
    }
    else if (fstat(fd, &st)<0) 
    {
        /* Make sure the length still matches, now that we
         * opened the file :/ */
        LOG_PRINT(LOG_ERROR, "fstat failed.");
        goto err;
    }
    else
    {
        LOG_PRINT(LOG_INFO, "Got the file!");
        evbuffer_add_file(evb, fd, 0, st.st_size);
    }
    result = 1;
    goto done;

err:
    if(fd != -1)
        close(fd);

done:
    if(origName)
        free(origName);
    if(dir)
        closedir(dir);
    if (whole_path)
        free(whole_path);
    return result;
}

