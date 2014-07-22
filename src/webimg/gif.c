#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <gif_lib.h>

#include "webimg2.h"
#include "internal.h"

#define WI_GIF_COLOR_BITS				8
static int wi_gif_color_depth			= 1 << WI_GIF_COLOR_BITS;

int wi_is_gif(void *data, size_t len)
{
	if (len <= GIF_STAMP_LEN) return 0;

	return (strncmp((char *)data, GIF87_STAMP, GIF_STAMP_LEN) == 0
			|| strncmp((char *)data, GIF89_STAMP, GIF_STAMP_LEN) == 0
		   );
}

struct wi_gif_arg {
	GifFileType	*gif;
};

/* read from blob */
int wi_gif_input_fb(GifFileType *gif, GifByteType *data, int size)
{
	size_t left, read;
	struct image *im;

	im = (struct image *)(gif->UserData);
	left = blob_left(&im->in_buf);
	read = (left > size) ? size : left;
	if (read <= 0) return read;

	memcpy(data, im->in_buf.ptr, read);
	im->in_buf.ptr += read;

	return read;
}

int wi_gif_init_read(struct image *im)
{
	GifFileType *gif;

	/*
	gif = (GifFileType *)MALLOC(im, sizeof(GifFileType *));
	if (gif == NULL) return -1;
	*/

	gif = DGifOpen((void *)im, wi_gif_input_fb);
	if (gif == NULL) {
		//FREE(im, gif);
		return -1;
	}

	im->load_arg = gif;

	return 0;
}

int wi_gif_read_img_info(struct image *im)
{
	GifFileType *gif;

	gif = im->load_arg;

	im->cols = gif->SWidth;
	im->rows = gif->SHeight;
	
	return 0;
}

/* only decode the 1st frame */
int wi_gif_load_image(struct image *im)
{
	int ret;
	int i, j, k;
	int /*bg, */trans = -1;
	uint8_t *ptr;
	GifFileType *gif;
	GifRecordType type;
	GifPixelType *line;
	unsigned int cols, rows;
	ColorMapObject *color_map;
	GifColorType *color/*, *bgcolor*/, white = {0xff, 0xff, 0xff};

	gif = im->load_arg;

	while (1) {
		ret = DGifGetRecordType(gif, &type);
		if (ret == GIF_ERROR) return -1;
		if (type == TERMINATE_RECORD_TYPE) return -1;
		/* read all extension record */
		if (type == EXTENSION_RECORD_TYPE) {
			int code;
			GifByteType *ext;
			ext = NULL;

			DGifGetExtension(gif, &code, &ext);
			while (ext) {
				if ((code == 0xf9) && (ext[1] & 1) && (trans < 0)) {
					trans = (int)ext[4];
				}
				ext = NULL;
				DGifGetExtensionNext(gif, &ext);
			}
			continue;
		}
		if (type != IMAGE_DESC_RECORD_TYPE) continue;

		/* read the desc record */
		ret = DGifGetImageDesc(gif);
		if (ret == GIF_ERROR) return -1;

		cols = gif->Image.Width;
		rows = gif->Image.Height;
		if (!wi_is_valid_size(cols, rows)) return -1;
		if (cols > im->cols) cols = im->cols;
		if (rows > im->rows) rows = im->rows;

		/* select pallet */
		color_map = gif->Image.ColorMap ? gif->Image.ColorMap : gif->SColorMap;
		/*
		bg = gif->SBackGroundColor;
		bgcolor = color_map->Colors + bg;
		if (bgcolor->Red == 0 && bgcolor->Green == 0 && bgcolor->Blue == 0) {
			bgcolor = &white;
		}
		*/

		line = (GifPixelType *)MALLOC(im, sizeof(GifPixelType) * cols);
		if (line == NULL) {
			return -1;
		}

		ret = image_alloc_data(im);
		if (ret != 0) {
			FREE(im, line);
			return -1;
		}

#define get_background(trans, c) ((trans == (c)) ? &white : color_map->Colors+(c))
		if (gif->Image.Interlace) {
			static int start[] = {0, 4, 2, 1};
			static int jump[] = {8, 8, 4, 2};

			for (i=0; i<4; i++) {
				for (j=start[i]; j<rows; j+=jump[i]) {
					DGifGetLine(gif, line, cols);
					ptr = im->row[j];
					for (k=0; k<cols; k++) {
						color = get_background(trans, line[k]);
						*ptr++ = color->Red;
						*ptr++ = color->Green;
						*ptr++ = color->Blue;
					}
					for (k=cols; k<im->cols; k++) {
						*ptr++ = 0xff;
						*ptr++ = 0xff;
						*ptr++ = 0xff;
					}
				}
			}
		} else {
			for (i=0; i<rows; i++) {
				DGifGetLine(gif, line, cols);
				ptr = im->row[i];
				for (j=0; j<cols; j++) {
					color = get_background(trans, line[j]);
					*ptr++ = color->Red;
					*ptr++ = color->Green;
					*ptr++ = color->Blue;
				}
				for (j=cols; j<im->cols; j++) {
					*ptr++ = 0xff;
					*ptr++ = 0xff;
					*ptr++ = 0xff;
				}
			}
		}
		for (i=rows; i<im->rows; i++) {
			ptr = im->row[i];
			for (j=0; j<im->cols; j++) {
				*ptr++ = 0xff;
				*ptr++ = 0xff;
				*ptr++ = 0xff;
			}
		}

		break;
	}

	FREE(im, line);

	im->colorspace = CS_RGB;
	im->quality = WI_DEFAULT_QUALITY;
	im->loaded = 1;

	return 0;
}

void wi_gif_finish_read(struct image *im)
{
	GifFileType *gif;

	gif = im->load_arg;
	if (gif == NULL) return;

	DGifCloseFile(gif);

	//FREE(im, gif);
	im->load_arg = NULL;

	return;
}

int wi_gif_output_tb(GifFileType *gif, const GifByteType *data, int size)
{
	int ret;
	struct image *im;

	assert(size > 0);

	im = gif->UserData;
	ret = expand_blob(im, &im->ou_buf, im->ou_buf.len + size);
	if (ret != 0) {
		return 0;
	}
	memcpy(im->ou_buf.ptr, data, size);
	im->ou_buf.ptr += size;

	return size;
}

/******************** START: from giflib by Gershon Elber ******************/
#define COLOR_ARRAY_SIZE 32768
#define BITS_PER_PRIM_COLOR 5
#define MAX_PRIM_COLOR      0x1f

//buaazp: maybe wrong here
static int SortRGBAxis = 2;

typedef struct QuantizedColorType {
    GifByteType RGB[3];
    GifByteType NewColorIndex;
    long Count;
    struct QuantizedColorType *Pnext;
} QuantizedColorType;

typedef struct NewColorMapType {
    GifByteType RGBMin[3], RGBWidth[3];
    unsigned int NumEntries; /* # of QuantizedColorType in linked list below */
    unsigned long Count; /* Total number of pixels in all the entries */
    QuantizedColorType *QuantizedColors;
} NewColorMapType;

static int
SortCmpRtn(const VoidPtr Entry1,
           const VoidPtr Entry2) {

    return (*((QuantizedColorType **) Entry1))->RGB[SortRGBAxis] -
       (*((QuantizedColorType **) Entry2))->RGB[SortRGBAxis];
}

static int
SubdivColorMap(NewColorMapType * NewColorSubdiv,
               unsigned int ColorMapSize,
               unsigned int *NewColorMapSize) {

    int MaxSize;
    unsigned int i, j, Index = 0, NumEntries, MinColor, MaxColor;
    long Sum, Count;
    QuantizedColorType *QuantizedColor, **SortArray;

    while (ColorMapSize > *NewColorMapSize) {
        /* Find candidate for subdivision: */
        MaxSize = -1;
        for (i = 0; i < *NewColorMapSize; i++) {
            for (j = 0; j < 3; j++) {
                if ((((int)NewColorSubdiv[i].RGBWidth[j]) > MaxSize) &&
                      (NewColorSubdiv[i].NumEntries > 1)) {
                    MaxSize = NewColorSubdiv[i].RGBWidth[j];
                    Index = i;
                    //SortRGBAxis = j;
                }
            }
        }

        if (MaxSize == -1)
            return GIF_OK;

        /* Split the entry Index into two along the axis SortRGBAxis: */

        /* Sort all elements in that entry along the given axis and split at
         * the median.  */
        SortArray = (QuantizedColorType **)malloc(
                      sizeof(QuantizedColorType *) * 
                      NewColorSubdiv[Index].NumEntries);
        if (SortArray == NULL)
            return GIF_ERROR;
        for (j = 0, QuantizedColor = NewColorSubdiv[Index].QuantizedColors;
             j < NewColorSubdiv[Index].NumEntries && QuantizedColor != NULL;
             j++, QuantizedColor = QuantizedColor->Pnext)
            SortArray[j] = QuantizedColor;

		/*
        qsort(SortArray, NewColorSubdiv[Index].NumEntries,
              sizeof(QuantizedColorType *), SortCmpRtn);
			  */
        qsort(SortArray, j,
              sizeof(QuantizedColorType *), SortCmpRtn);

        /* Relink the sorted list into one: */
        for (j = 0; j < NewColorSubdiv[Index].NumEntries - 1; j++)
            SortArray[j]->Pnext = SortArray[j + 1];
        SortArray[NewColorSubdiv[Index].NumEntries - 1]->Pnext = NULL;
        NewColorSubdiv[Index].QuantizedColors = QuantizedColor = SortArray[0];
        free((char *)SortArray);

        /* Now simply add the Counts until we have half of the Count: */
        Sum = NewColorSubdiv[Index].Count / 2 - QuantizedColor->Count;
        NumEntries = 1;
        Count = QuantizedColor->Count;
        while ((Sum -= QuantizedColor->Pnext->Count) >= 0 &&
               QuantizedColor->Pnext != NULL &&
               QuantizedColor->Pnext->Pnext != NULL) {
            QuantizedColor = QuantizedColor->Pnext;
            NumEntries++;
            Count += QuantizedColor->Count;
        }
        /* Save the values of the last color of the first half, and first
         * of the second half so we can update the Bounding Boxes later.
         * Also as the colors are quantized and the BBoxes are full 0..255,
         * they need to be rescaled.
         */
        MaxColor = QuantizedColor->RGB[SortRGBAxis]; /* Max. of first half */
        MinColor = QuantizedColor->Pnext->RGB[SortRGBAxis]; /* of second */
        MaxColor <<= (8 - BITS_PER_PRIM_COLOR);
        MinColor <<= (8 - BITS_PER_PRIM_COLOR);

        /* Partition right here: */
        NewColorSubdiv[*NewColorMapSize].QuantizedColors =
           QuantizedColor->Pnext;
        QuantizedColor->Pnext = NULL;
        NewColorSubdiv[*NewColorMapSize].Count = Count;
        NewColorSubdiv[Index].Count -= Count;
        NewColorSubdiv[*NewColorMapSize].NumEntries =
           NewColorSubdiv[Index].NumEntries - NumEntries;
        NewColorSubdiv[Index].NumEntries = NumEntries;
        for (j = 0; j < 3; j++) {
            NewColorSubdiv[*NewColorMapSize].RGBMin[j] =
               NewColorSubdiv[Index].RGBMin[j];
            NewColorSubdiv[*NewColorMapSize].RGBWidth[j] =
               NewColorSubdiv[Index].RGBWidth[j];
        }
        NewColorSubdiv[*NewColorMapSize].RGBWidth[SortRGBAxis] =
           NewColorSubdiv[*NewColorMapSize].RGBMin[SortRGBAxis] +
           NewColorSubdiv[*NewColorMapSize].RGBWidth[SortRGBAxis] - MinColor;
        NewColorSubdiv[*NewColorMapSize].RGBMin[SortRGBAxis] = MinColor;

        NewColorSubdiv[Index].RGBWidth[SortRGBAxis] =
           MaxColor - NewColorSubdiv[Index].RGBMin[SortRGBAxis];

        (*NewColorMapSize)++;
    }

    return GIF_OK;
}

static int QuantizeBufferRGB
(uint32_t Width, uint32_t Height, int *ColorMapSize, uint8_t *data,
GifByteType * OutputBuffer, GifColorType * OutputColorMap)
{
    unsigned int Index, NumOfEntries;
    int i, j, MaxRGBError[3];
    unsigned int NewColorMapSize;
    long Red, Green, Blue;
    NewColorMapType NewColorSubdiv[256];
    QuantizedColorType *ColorArrayEntries, *QuantizedColor;

	/*
    ColorArrayEntries = (QuantizedColorType *)malloc(
                           sizeof(QuantizedColorType) * COLOR_ARRAY_SIZE);
						   */
    ColorArrayEntries = (QuantizedColorType *)calloc( COLOR_ARRAY_SIZE, 
                           sizeof(QuantizedColorType));
    if (ColorArrayEntries == NULL) {
        return GIF_ERROR;
    }

    for (i = 0; i < COLOR_ARRAY_SIZE; i++) {
        ColorArrayEntries[i].RGB[0] = i >> (2 * BITS_PER_PRIM_COLOR);
        ColorArrayEntries[i].RGB[1] = (i >> BITS_PER_PRIM_COLOR) & MAX_PRIM_COLOR;
        ColorArrayEntries[i].RGB[2] = i & MAX_PRIM_COLOR;
        ColorArrayEntries[i].Count = 0;
    }

	struct wi_rgb__ {
		uint8_t r;
		uint8_t g;
		uint8_t b;
	} *ptr = (struct wi_rgb__ *)data;

    /* Sample the colors and their distribution: */
    for (i = 0; i < (int)(Width * Height); i++) {
        Index = ((ptr->r >> (8 - BITS_PER_PRIM_COLOR)) <<
                  (2 * BITS_PER_PRIM_COLOR)) +
                ((ptr->g >> (8 - BITS_PER_PRIM_COLOR)) <<
                  BITS_PER_PRIM_COLOR) +
                (ptr->b >> (8 - BITS_PER_PRIM_COLOR));
		ptr++;
        ColorArrayEntries[Index].Count++;
    }

    /* Put all the colors in the first entry of the color map, and call the
     * recursive subdivision process.  */
    for (i = 0; i < 256; i++) {
        NewColorSubdiv[i].QuantizedColors = NULL;
        NewColorSubdiv[i].Count = NewColorSubdiv[i].NumEntries = 0;
        for (j = 0; j < 3; j++) {
            NewColorSubdiv[i].RGBMin[j] = 0;
            NewColorSubdiv[i].RGBWidth[j] = 255;
        }
    }

    /* Find the non empty entries in the color table and chain them: */
    for (i = 0; i < COLOR_ARRAY_SIZE; i++)
        if (ColorArrayEntries[i].Count > 0)
            break;
    QuantizedColor = NewColorSubdiv[0].QuantizedColors = &ColorArrayEntries[i];
    NumOfEntries = 1;
    while (++i < COLOR_ARRAY_SIZE)
        if (ColorArrayEntries[i].Count > 0) {
            QuantizedColor->Pnext = &ColorArrayEntries[i];
            QuantizedColor = &ColorArrayEntries[i];
            NumOfEntries++;
        }
    QuantizedColor->Pnext = NULL;

    NewColorSubdiv[0].NumEntries = NumOfEntries; /* Different sampled colors */
    NewColorSubdiv[0].Count = ((long)Width) * Height; /* Pixels */
    NewColorMapSize = 1;
    if (SubdivColorMap(NewColorSubdiv, *ColorMapSize, &NewColorMapSize) !=
       GIF_OK) {
        free((char *)ColorArrayEntries);
        return GIF_ERROR;
    }
    if (NewColorMapSize < *ColorMapSize) {
        /* And clear rest of color map: */
        for (i = NewColorMapSize; i < *ColorMapSize; i++)
            OutputColorMap[i].Red = OutputColorMap[i].Green =
                OutputColorMap[i].Blue = 0;
    }

    /* Average the colors in each entry to be the color to be used in the
     * output color map, and plug it into the output color map itself. */
    for (i = 0; i < NewColorMapSize; i++) {
        if ((j = NewColorSubdiv[i].NumEntries) > 0) {
            QuantizedColor = NewColorSubdiv[i].QuantizedColors;
            Red = Green = Blue = 0;
            while (QuantizedColor) {
                QuantizedColor->NewColorIndex = i;
                Red += QuantizedColor->RGB[0];
                Green += QuantizedColor->RGB[1];
                Blue += QuantizedColor->RGB[2];
                QuantizedColor = QuantizedColor->Pnext;
            }
            OutputColorMap[i].Red = (Red << (8 - BITS_PER_PRIM_COLOR)) / j;
            OutputColorMap[i].Green = (Green << (8 - BITS_PER_PRIM_COLOR)) / j;
            OutputColorMap[i].Blue = (Blue << (8 - BITS_PER_PRIM_COLOR)) / j;
        }
    }

	ptr = (struct wi_rgb__ *)data;
#define ABS(x)    ((x) > 0 ? (x) : (-(x)))
    /* Finally scan the input buffer again and put the mapped index in the
     * output buffer.  */
    MaxRGBError[0] = MaxRGBError[1] = MaxRGBError[2] = 0;
    for (i = 0; i < (int)(Width * Height); i++) {
        Index = ((ptr->r >> (8 - BITS_PER_PRIM_COLOR)) <<
                 (2 * BITS_PER_PRIM_COLOR)) +
                ((ptr->g >> (8 - BITS_PER_PRIM_COLOR)) <<
                 BITS_PER_PRIM_COLOR) +
                (ptr->b >> (8 - BITS_PER_PRIM_COLOR));
        Index = ColorArrayEntries[Index].NewColorIndex;
        OutputBuffer[i] = Index;
        if (MaxRGBError[0] < ABS(OutputColorMap[Index].Red - ptr->r))
            MaxRGBError[0] = ABS(OutputColorMap[Index].Red - ptr->r);
        if (MaxRGBError[1] < ABS(OutputColorMap[Index].Green - ptr->g))
            MaxRGBError[1] = ABS(OutputColorMap[Index].Green - ptr->g);
        if (MaxRGBError[2] < ABS(OutputColorMap[Index].Blue - ptr->b))
            MaxRGBError[2] = ABS(OutputColorMap[Index].Blue - ptr->b);
		ptr++;
    }

    free((char *)ColorArrayEntries);

    *ColorMapSize = NewColorMapSize;

    return GIF_OK;
}
/****************************** END **************************************/

int wi_gif_save(struct image *im)
{
	int i, ret;
	GifFileType *gif;
	ColorMapObject *map;
	GifByteType *ptr, *buf;
	int map_size = wi_gif_color_depth;

	map = MakeMapObject(map_size, NULL);
	if (map == NULL) return -1;

	buf = MALLOC(im, im->rows * im->cols * sizeof(GifByteType));
	if (buf == NULL) {
		FreeMapObject(map);
		return -1;
	}

	if (im->colorspace == CS_RGB) {
		ret = QuantizeBufferRGB(im->cols, im->rows, &map_size,
				im->data, buf, map->Colors);
	} else if (im->colorspace == CS_GRAYSCALE) {
		ret = QuantizeBuffer(im->cols, im->rows, &map_size,
				im->data, im->data, im->data,
				buf, map->Colors);
	} else {
		FREE(im, buf);
		FreeMapObject(map);
		return -1;
	}

	if (ret == GIF_ERROR) {
		FREE(im, buf);
		FreeMapObject(map);
		return -1;
	}

	gif = EGifOpen((void *)im, wi_gif_output_tb);
	if (gif == NULL) {
		FREE(im, buf);
		FreeMapObject(map);
		return -1;
	}

	ret = (EGifPutScreenDesc(gif, im->cols, im->rows, WI_GIF_COLOR_BITS, 0, map)
		== GIF_ERROR);
	ret = ret || (EGifPutImageDesc(gif, 0, 0, im->cols, im->rows, FALSE, NULL)
		== GIF_ERROR);
	if (ret) {
		FREE(im, buf);
		FreeMapObject(map);
		EGifCloseFile(gif);
		return -1;
	}

	ptr = buf;
	for (i=0; i<im->rows; i++) {
		ret = EGifPutLine(gif, ptr, im->cols);
		if (ret == GIF_ERROR) {
			FREE(im, buf);
			FreeMapObject(map);
			EGifCloseFile(gif);
			return -1;
		}
		ptr += im->cols;
	}

	FREE(im, buf);
	FreeMapObject(map);
	EGifCloseFile(gif);

	return 0;
}

struct loader gif_loader = {
	.name			= "GIF",
	.ext			= ".gif",
	.mime_type		= "image/gif",

	.suitable		= wi_is_gif,
	.init			= wi_gif_init_read,
	.read_info		= wi_gif_read_img_info,
	.resample		= NULL,
	.load			= wi_gif_load_image,
	.save			= wi_gif_save,

	.finish			= wi_gif_finish_read,
};
