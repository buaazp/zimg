#include <stdio.h>
#include <stdlib.h>
#include <wand/MagickWand.h>


int main()
{
    int i;
    for(i = 0; i < 1; i++)
    {
        MagickWand *magick_wand;
        MagickWandGenesis();
        magick_wand = NewMagickWand();

        const char *rspPath = "./5f189.jpeg";
        MagickReadImage(magick_wand, rspPath);
        MagickSizeType imgSize;
        MagickGetImageLength(magick_wand, &imgSize);
        printf("Got Image Size: %d\n", (int)imgSize);

        magick_wand=DestroyMagickWand(magick_wand);
        MagickWandTerminus();
    }
    return 0;
}
