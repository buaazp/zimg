#include <wand/MagickWand.h>
#include "zimg.h"

char *getImg(char *path, int width, int height, int scaled)
{
    DIR *d;
    struct dirent *ent;
    char *imgType;
    char *imgPath = NULL; 
    char name[MAX_LINE];
    char *origName = NULL;
    char *origPath;
    int len;
    int find = 0;
    sprintf(name, "%d*%d", width, height);

    if (!(d = opendir(path)))
    {
        perror("dir open failed.");
        return NULL;
    }

    while(ent = readdir(d))
    {
        const char *tmpName = ent->d_name;
        if(strstr(tmpName, name) == tmpName && strchr(tmpName, '.') == tmpName + strlen(name))
        {
            find = 1;
            strcpy(name, tmpName);
        }
        else if(strstr(tmpName, "orig") == tmpName)
        {
            len = strlen(tmpName) + 1;
            origName = malloc(len);
            strcpy(origName, tmpName);
            imgType = strchr(origName, '.');
            imgType++;
            continue;
        }
    }
    if(find == 0 && origName != NULL)
    {
        char *newPath;
        len = strlen(path) + strlen(name) + strlen(imgType) + 3;
        newPath = malloc(len);
        evutil_snprintf(newPath, len, "%s/%s.%s", path, name, imgType);

        len = strlen(path) + strlen(origName) + 2;
        origPath = malloc(len);
        evutil_snprintf(origPath, len, "%s/%s", path, origName);

        if(width == 0 || height == 0)
        {
            DEBUG_PRINT("Return original image.");
            return origPath;
        }
        else
        {
            if(imgResize(origPath, newPath, width, height, scaled) == -1)
            {
                perror("Resize img failed.");
                return NULL;
            }
            else
            {
                sprintf(name, "%s.%s", name, imgType);
                find = 1;
            }
        }
    }
    if(find == 1)
    {
        len = strlen(path)+strlen(name)+2;
        if (!(imgPath= malloc(len))) {
            perror("malloc failed.");
            return NULL;
        }
        evutil_snprintf(imgPath, len, "%s/%s", path, name);
    }
    return imgPath;
}

int imgResize(char *img, char *imgNew, int width, int height, int scaled)
{
#define ThrowWandException(wand) \
    { \
        char \
        *description; \
        \
        ExceptionType \
        severity; \
        \
        description=MagickGetException(wand,&severity); \
        (void) fprintf(stderr,"%s %s %lu %s\n",GetMagickModule(),description); \
        description=(char *) MagickRelinquishMemory(description); \
        exit(-1); \
    }
    int result = 0;

    MagickBooleanType
        status;

    MagickWand
        *magick_wand;
    /*
       Read an image.
       */
    MagickWandGenesis();
    magick_wand=NewMagickWand();
    status=MagickReadImage(magick_wand, img);
    if (status == MagickFalse)
    {
        ThrowWandException(magick_wand);
        result = -1;
    }
    /*
       Turn the images into a thumbnail sequence.
       */
    if(scaled == 1)
    {
        int owidth = MagickGetImageWidth(magick_wand);
        float oheight = MagickGetImageHeight(magick_wand);
        height = width * oheight / owidth;
    }
    MagickResetIterator(magick_wand);
    while (MagickNextImage(magick_wand) != MagickFalse)
        MagickResizeImage(magick_wand, width, height, LanczosFilter, 1.0);
    /*
       Write the image then destroy it.
       */
    status=MagickWriteImages(magick_wand, imgNew, MagickTrue);
    if (status == MagickFalse)
    {
        ThrowWandException(magick_wand);
        result = -1;
    }
    magick_wand=DestroyMagickWand(magick_wand);
    MagickWandTerminus();
    return result;
}
