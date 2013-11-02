#include <stdio.h>
#include <stdlib.h>
#include <wand/MagickWand.h>


int main()
{
    int i;
    for(i = 0; i < 1; i++)
    {
        MagickWand *magick_wand;
        MagickBooleanType status;
        MagickSizeType imgSize;
        int width = 100, height = 100;
        MagickWandGenesis();
        magick_wand = NewMagickWand();

        const char *rspPath = "./5f189.jpeg";
        status = MagickReadImage(magick_wand, rspPath);
        if(status == MagickFalse)
        {
            printf("Image[] Read Failed!\n");
        }
        else
        {
            MagickGetImageLength(magick_wand, &imgSize);
            printf("Got Image Size: %d\n", (int)imgSize);

            status = MagickResizeImage(magick_wand, width, height, LanczosFilter, 1.0);
            if(status == MagickFalse)
            {
                printf("Image[%s] Resize Failed!\n", rspPath);
            }
            else
            {
                printf("Resize Successfully!.\n");
                status = MagickWriteImage(magick_wand, "./new.jpeg");
                if(status == MagickFalse)
                {
                    printf("Image[] Write Failed!\n");
                }
                else
                {
                    printf("Write Successfully!.\n");
                }
            }
        }

        magick_wand=DestroyMagickWand(magick_wand);
        MagickWandTerminus();
    }
    return 0;
}
