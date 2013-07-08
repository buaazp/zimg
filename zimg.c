#include "zcommon.h"
#include "zmd5.h"

int save_img(const char *type, const char *buff, int len, char *md5)
{
    LOG_PRINT(LOG_INFO, "Begin to Caculate MD5...");
    md5_state_t mdctx;
    md5_byte_t md_value[16];
    char md5sum[33];
    unsigned int md_len, i;
    int h, l;
    int fd = -1;
    int wlen = 0;
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
    char *origName = (char *)malloc(256);
    sprintf(savePath, "%s/%s", _img_path, md5sum);
    LOG_PRINT(LOG_INFO, "savePath: %s", savePath);
    if(isDir(savePath) == -1)
    {
        if(mkDir(savePath) == -1)
        {
            LOG_PRINT(LOG_ERROR, "savePath[%s] Create Failed!", savePath);
            goto err;
        }
    }

    sprintf(origName, "0*0.%s", type);
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
        close(fd);
        goto err;
    }
    else if(wlen < len)
    {
        LOG_PRINT(LOG_ERROR, "Only part of data is been writed.");
        close(fd);
        goto err;
    }
    if(fd != -1)
    {
        close(fd);
    }
    LOG_PRINT(LOG_INFO, "Image [%s] Write Successfully!", saveName);

    // to gen cacheKey like this: rspPath-/926ee2f570dc50b2575e35a6712b08ce
    char *cacheKey = (char *)malloc(strlen(md5sum) + 10);
    sprintf(cacheKey, "rspPath-/%s", md5sum);
    set_cache(cacheKey, saveName);
    free(cacheKey);

    return 1;


err:
    if(savePath)
        free(savePath);
    if(saveName)
        free(saveName);
    if(origName)
        free(origName);
    return -1;
}
