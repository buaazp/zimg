#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <setjmp.h>
#include <jpeglib.h>
#include <jerror.h>

#include "webimg2.h"
#include "internal.h"

#include "../zlog.h"

//#define MAX_COMPONENTS			4
#define WI_JPEG_QUALITY			85 
#define MIN_OUTBUFF_LEN			8192
#define JPEG_MAGIC				"\377\330\377"

struct wi_jpeg_error_mgr {
	struct jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
};

struct wi_jpeg_arg {
	int								components;
	jpeg_component_info				comp_info[MAX_COMPONENTS];
	struct wi_jpeg_error_mgr		jerr;
	struct jpeg_decompress_struct	jinfo;
};

static void wi_jpeg_exit(j_common_ptr jinfo)
{
	struct wi_jpeg_error_mgr *err;
	err = (struct wi_jpeg_error_mgr *)jinfo->err;

	siglongjmp(err->setjmp_buffer, 1);
	return;
}

static void wi_jpeg_eh_null(j_common_ptr jinfo)
{
	return;
}

static void wi_jpeg_eh_null2(j_common_ptr jinfo, int msg)
{
	return;
}
static void wi_jpeg_eh_null3(j_common_ptr jinfo, char *buf)
{
	return;
}

static void wi_jpeg_set_error_handler(
		j_common_ptr jinfo, struct wi_jpeg_error_mgr *jerr)
{
	jinfo->err = jpeg_std_error(&(jerr->pub));
	jerr->pub.error_exit = wi_jpeg_exit;
	jerr->pub.output_message = wi_jpeg_eh_null;
	jerr->pub.emit_message = wi_jpeg_eh_null2;
	jerr->pub.format_message  = wi_jpeg_eh_null3;
	return;
}

struct wi_jpeg_src {
	struct jpeg_source_mgr pub;
	struct 	image *im;
	boolean	start_of_blob;
};

struct wi_jpeg_dst {
	struct	jpeg_destination_mgr pub;
	struct	image *im;
	//JOCTET	*buffer;
};

int wi_is_jpeg(void *data, size_t len)
{
	if (len <= 3) return 0;
	return (!memcmp(data, JPEG_MAGIC, 3));
}

void wi_jpeg_init_source(j_decompress_ptr jinfo)
{
	struct wi_jpeg_src *src;

	src = (struct wi_jpeg_src *)jinfo->src;
	src->start_of_blob = TRUE;
}

static JOCTET wi_jpeg_src_buf_term[2] = {0xff, JPEG_EOI};

boolean wi_jpeg_fill_input_buffer(j_decompress_ptr jinfo)
{
	size_t left;
	struct wi_jpeg_src *src;

	src = (struct wi_jpeg_src *)jinfo->src;

	left = blob_left(&src->im->in_buf);
	if (left <= 0) {
		if (src->start_of_blob) ERREXIT(jinfo, JERR_INPUT_EMPTY);
		src->pub.next_input_byte = wi_jpeg_src_buf_term;
		src->pub.bytes_in_buffer=2;
		return (TRUE);
	}

	src->pub.bytes_in_buffer = left;
	src->pub.next_input_byte = (JOCTET *)(src->im->in_buf.ptr);
	src->im->in_buf.ptr += left;
	src->start_of_blob = FALSE;

	return (TRUE);
}

void wi_jpeg_skip_input_data(j_decompress_ptr jinfo, long num_bytes)
{
	struct wi_jpeg_src *src;

	src = (struct wi_jpeg_src *)jinfo->src;

	if (num_bytes <= 0) return;
	while (num_bytes > (long)src->pub.bytes_in_buffer) {
		num_bytes -= (long)src->pub.bytes_in_buffer;
		wi_jpeg_fill_input_buffer(jinfo);
	}
	src->pub.next_input_byte += (size_t)num_bytes;
	src->pub.bytes_in_buffer -= (size_t)num_bytes;
}

void wi_jpeg_term_source(j_decompress_ptr jinfo)
{
	return;
}

void wi_jpeg_blob_src(j_decompress_ptr jinfo, struct image *im)
{
	struct wi_jpeg_src *src;

	jinfo->src = (struct jpeg_source_mgr *) (*jinfo->mem->alloc_small)
		((j_common_ptr)jinfo, JPOOL_PERMANENT, sizeof(struct wi_jpeg_src));
	src = (struct wi_jpeg_src *)jinfo->src;

	src->pub.init_source = wi_jpeg_init_source;
	src->pub.fill_input_buffer = wi_jpeg_fill_input_buffer;
	src->pub.skip_input_data = wi_jpeg_skip_input_data;
	src->pub.resync_to_restart = jpeg_resync_to_restart;
	src->pub.term_source = wi_jpeg_term_source;

	src->im = im;
	src->pub.bytes_in_buffer = 0;
	src->pub.next_input_byte = NULL;
}

int wi_jpeg_init_read(struct image *im)
{
	int i;
	struct wi_jpeg_arg *arg;

	arg = (struct wi_jpeg_arg *)MALLOC(im, sizeof(struct wi_jpeg_arg));
	if (arg == NULL) return -1;

	/* init compontents */
	for (i=0; i<MAX_COMPONENTS; i++) {
		arg->comp_info[i].component_id	= i;
		arg->comp_info[i].h_samp_factor	= 1;
		arg->comp_info[i].v_samp_factor	= 1;
	}   
	arg->components = MAX_COMPONENTS;

	wi_jpeg_set_error_handler((j_common_ptr)&arg->jinfo, &arg->jerr);
	if (sigsetjmp(arg->jerr.setjmp_buffer, 1)) {
		FREE(im, arg);
		return -1;
	}

	jpeg_create_decompress(&arg->jinfo);

	rewind_blob(&im->in_buf);

	wi_jpeg_blob_src(&arg->jinfo, im);
    LOG_PRINT(LOG_INFO, "[init_read_blob]arg->jinfo->jpeg_color_space = %d", arg->jinfo.jpeg_color_space);
    LOG_PRINT(LOG_INFO, "[init_read_blob]arg->jinfo->out_color_space = %d", arg->jinfo.out_color_space);

	im->load_arg = arg;

	return 0;
}

/***********************
 * this function comes from ImageMagick, here is the license:
 * http://www.imagemagick.org/script/license.php
 **/
static uint32_t wi_jpeg_get_quality(struct wi_jpeg_arg *arg)/*{{{*/
{
	uint32_t quality = 0;
	JQUANT_TBL **tbls;
	tbls = arg->jinfo.quant_tbl_ptrs;

	static ssize_t
	hashA[101] =
	{
		1020, 1015,  932,  848,  780,  735,  702,  679,  660,  645,
		632,  623,  613,  607,  600,  594,  589,  585,  581,  571,
		555,  542,  529,  514,  494,  474,  457,  439,  424,  410,
		397,  386,  373,  364,  351,  341,  334,  324,  317,  309,
		299,  294,  287,  279,  274,  267,  262,  257,  251,  247,
		243,  237,  232,  227,  222,  217,  213,  207,  202,  198,
		192,  188,  183,  177,  173,  168,  163,  157,  153,  148,
		143,  139,  132,  128,  125,  119,  115,  108,  104,   99,
		94,   90,   84,   79,   74,   70,   64,   59,   55,   49,
		45,   40,   34,   30,   25,   20,   15,   11,	6,	4,
		0
	},
	sumsA[101] =
	{
		32640, 32635, 32266, 31495, 30665, 29804, 29146, 28599, 28104,
		27670, 27225, 26725, 26210, 25716, 25240, 24789, 24373, 23946,
		23572, 22846, 21801, 20842, 19949, 19121, 18386, 17651, 16998,
		16349, 15800, 15247, 14783, 14321, 13859, 13535, 13081, 12702,
		12423, 12056, 11779, 11513, 11135, 10955, 10676, 10392, 10208,
		9928,  9747,  9564,  9369,  9193,  9017,  8822,  8639,  8458,
		8270,  8084,  7896,  7710,  7527,  7347,  7156,  6977,  6788,
		6607,  6422,  6236,  6054,  5867,  5684,  5495,  5305,  5128,
		4945,  4751,  4638,  4442,  4248,  4065,  3888,  3698,  3509,
		3326,  3139,  2957,  2775,  2586,  2405,  2216,  2037,  1846,
		1666,  1483,  1297,  1109,   927,   735,   554,   375,   201,
		128,	 0
	},
	hashB[101] =
	{
		510,  505,  422,  380,  355,  338,  326,  318,  311,  305,
		300,  297,  293,  291,  288,  286,  284,  283,  281,  280,
		279,  278,  277,  273,  262,  251,  243,  233,  225,  218,
		211,  205,  198,  193,  186,  181,  177,  172,  168,  164,
		158,  156,  152,  148,  145,  142,  139,  136,  133,  131,
		129,  126,  123,  120,  118,  115,  113,  110,  107,  105,
		102,  100,   97,   94,   92,   89,   87,   83,   81,   79,
		76,   74,   70,   68,   66,   63,   61,   57,   55,   52,
		50,   48,   44,   42,   39,   37,   34,   31,   29,   26,
		24,   21,   18,   16,   13,   11,	8,	6,	3,	2,
		0
	},
	sumsB[101] =
	{
		16320, 16315, 15946, 15277, 14655, 14073, 13623, 13230, 12859,
		12560, 12240, 11861, 11456, 11081, 10714, 10360, 10027,  9679,
		9368,  9056,  8680,  8331,  7995,  7668,  7376,  7084,  6823,
		6562,  6345,  6125,  5939,  5756,  5571,  5421,  5240,  5086,
		4976,  4829,  4719,  4616,  4463,  4393,  4280,  4166,  4092,
		3980,  3909,  3835,  3755,  3688,  3621,  3541,  3467,  3396,
		3323,  3247,  3170,  3096,  3021,  2952,  2874,  2804,  2727,
		2657,  2583,  2509,  2437,  2362,  2290,  2211,  2136,  2068,
		1996,  1915,  1858,  1773,  1692,  1620,  1552,  1477,  1398,
		1326,  1251,  1179,  1109,  1031,   961,   884,   814,   736,
		667,   592,   518,   441,   369,   292,   221,   151,	86,
		64,	 0
	};

	size_t i, j, qval, sum = 0;
	for (i=0; i<NUM_QUANT_TBLS; i++) {
		if (tbls[i] != NULL) {
			for (j=0; j<DCTSIZE2; j++) {
				sum += tbls[i]->quantval[j];
			}
		}
	}

	ssize_t *hash, *sums;
	if (tbls[0] != NULL && tbls[1] != NULL) {
		hash = hashA;
		sums = sumsA;
		qval = (ssize_t)(tbls[0]->quantval[2] + tbls[0]->quantval[53]
				+ tbls[1]->quantval[0] + tbls[1]->quantval[DCTSIZE2-1]);
	} else if (tbls[0] != NULL) {
		hash = hashB;
		sums = sumsB;
		qval = (ssize_t)(tbls[0]->quantval[2] + tbls[0]->quantval[53]);
	} else {
		return 0;
	}

	/* set image->quality */
	for (i=0; i<100; i++) {
		if (qval < hash[i] && sum < sums[i]) continue;
		if ((qval <= hash[i] && sum <= sums[i]) || i>=50) {
			quality = i+1;
		}
		break;
	}

	return quality;
}/*}}}*/

int wi_jpeg_read_img_info(struct image *im)/*{{{*/
{
	int i, comps;
	uint32_t quality;
	struct wi_jpeg_arg *arg;
	struct jpeg_decompress_struct * jinfo;

	arg		= im->load_arg;
	jinfo	= &arg->jinfo;

	/* set error handler */
	if (sigsetjmp(arg->jerr.setjmp_buffer, 1)) {
		jpeg_destroy_decompress(jinfo);
		FREE(im, arg);
		im->load_arg = NULL;
		return -1;
	}

	jpeg_read_header(jinfo, TRUE);

	/* set image attrs */
	im->cols		= jinfo->image_width;
	im->rows		= jinfo->image_height;

    LOG_PRINT(LOG_INFO, "[read_info]jinfo->jpeg_color_space = %d", jinfo->jpeg_color_space);
	if (jinfo->out_color_space == JCS_RGB) {
		im->colorspace = CS_RGB;
        LOG_PRINT(LOG_INFO, "[read_info]im->colorspace = CS_RGB");
	} else if (jinfo->out_color_space == JCS_GRAYSCALE) {
		im->colorspace = CS_GRAYSCALE;
        LOG_PRINT(LOG_INFO, "[read_info]im->colorspace = CS_GRAYSCALE");
	} else if (jinfo->out_color_space == JCS_CMYK) {
		im->colorspace = CS_CMYK;
	} else {
		return -1;
	}

	comps = jinfo->output_components;
    LOG_PRINT(LOG_INFO, "[read_info]jinfo->output_components = %d", jinfo->output_components);
	if (comps > MAX_COMPONENTS) {
		return -1;
	}
	for (i=0; i<comps; i++) {
		arg->comp_info[i] = jinfo->comp_info[i];
	}
	arg->components = comps;

	quality		= wi_jpeg_get_quality(arg);
	if (quality > 0) im->quality = quality;

	return 0;
}/*}}}*/

/* fast DCT scale*/
int wi_jpeg_resample(struct image *im, int num, int denom)
{
	struct wi_jpeg_arg *arg = im->load_arg;

	arg->jinfo.scale_num = num;
	arg->jinfo.scale_denom = denom;

	return 0;
}

static int wi_jpeg_rgb_to_gray(struct image *im)
{
    LOG_PRINT(LOG_INFO, "wi_jpeg_rgb_to_gray()...");
	int i, pixs;
	uint8_t r, g, b, *s, *d;
	struct wi_jpeg_arg *arg = im->load_arg;

	s = d = im->data;
	pixs = im->rows * im->cols;

	for (i=0; i<pixs/3; i++) {
		r = *s;
        s++;
		g = *s;
        s++;
		b = *s;
        s++;
		*d = (r + g + b) / 3;
        //LOG_PRINT(LOG_INFO, "%d/%d : r = %d g = %d b = %d d = %d", i, pixs, r, g, b, *d);
        d++;
        //s += 3;
	}

	s = im->data;
	pixs = im->cols;
	for (i=0; i<im->rows; i++) {
		im->row[i] = s;
		s += pixs;
	}

	im->colorspace = CS_GRAYSCALE;
	arg->components = 1;

	return 0;
}

/**
 * CMYK -> RGB
 * http://sourceforge.net/p/libjpeg-turbo/patches/15/
 */
static int wi_jpeg_cmyk_to_rgb(struct image *im)
{
	int i, pixs;
	uint8_t *s, *d;
	struct wi_jpeg_arg *arg = im->load_arg;

	s = d = im->data;
	pixs = im->rows * im->cols;

	for (i=0; i<pixs; i++) {
		*d++ = s[0]*s[3]/255;
		*d++ = s[1]*s[3]/255;
		*d++ = s[2]*s[3]/255;
		s += 4;
	}

	s = im->data;
	pixs = im->cols * 3;
	for (i=0; i<im->rows; i++) {
		im->row[i] = s;
		s += pixs;
	}

	im->colorspace = CS_RGB;
	arg->components = 3;

	return 0;
}


int wi_jpeg_load_image(struct image *im)
{
	int i, ret;
	struct wi_jpeg_arg *arg;
	struct jpeg_decompress_struct *jinfo;

	if (im->loaded)
    {
        LOG_PRINT(LOG_INFO, "loaded!");
        return 0;
    }

	/* start decompress image */
	arg = im->load_arg;
	jinfo = &arg->jinfo;
    /*
    jinfo->out_color_components = 1;
    jinfo->output_components = 1;
    LOG_PRINT(LOG_INFO, "arg->components = %d", arg->components);
    LOG_PRINT(LOG_INFO, "jinfo->num_components = %d", jinfo->num_components);
    LOG_PRINT(LOG_INFO, "jinfo->jpeg_color_space = %d", jinfo->jpeg_color_space);
    LOG_PRINT(LOG_INFO, "jinfo->out_color_space = %d", jinfo->out_color_space);
    LOG_PRINT(LOG_INFO, "jinfo->out_color_components = %d", jinfo->out_color_components);
    LOG_PRINT(LOG_INFO, "jinfo->output_components = %d", jinfo->output_components);
    LOG_PRINT(LOG_INFO, "jinfo->image_width = %d", jinfo->image_width);
    LOG_PRINT(LOG_INFO, "jinfo->image_height = %d", jinfo->image_height);
    LOG_PRINT(LOG_INFO, "jinfo->output_width = %d", jinfo->output_width);
    LOG_PRINT(LOG_INFO, "jinfo->output_height = %d", jinfo->output_height);
    */

	/* set jpeg error handler */
	if (sigsetjmp(arg->jerr.setjmp_buffer, 1)) {
		image_free_data(im);
		return -1;
	}

    LOG_PRINT(LOG_INFO, "jinfo->jpeg_color_space = %d", jinfo->jpeg_color_space);
    LOG_PRINT(LOG_INFO, "jinfo->out_color_space = %d", jinfo->out_color_space);
    LOG_PRINT(LOG_INFO, "jinfo->output_width = %d", jinfo->output_width);
    LOG_PRINT(LOG_INFO, "jinfo->output_height = %d", jinfo->output_height);
	jpeg_start_decompress(jinfo);
    LOG_PRINT(LOG_INFO, "jinfo->jpeg_color_space = %d", jinfo->jpeg_color_space);
    LOG_PRINT(LOG_INFO, "jinfo->out_color_space = %d", jinfo->out_color_space);
    LOG_PRINT(LOG_INFO, "jinfo->output_width = %d", jinfo->output_width);
    LOG_PRINT(LOG_INFO, "jinfo->output_height = %d", jinfo->output_height);
	im->cols = jinfo->output_width;
	im->rows = jinfo->output_height;

	ret = image_alloc_data(im);
	if (ret != 0) return -1;

	for (i=0; i<im->rows; i++) {
		jpeg_read_scanlines(jinfo, (JSAMPLE **)&(im->row[i]), 1);
	}

	if (im->colorspace == CS_CMYK) wi_jpeg_cmyk_to_rgb(im);
	//if (im->colorspace == CS_GRAYSCALE) wi_jpeg_rgb_to_gray(im);

	im->loaded = 1;

	return 0;
}

void wi_jpeg_finish_read(struct image *im)
{
	struct wi_jpeg_arg *arg;

	arg = (struct wi_jpeg_arg *)(im->load_arg);
	if (arg == NULL) return;

	if (sigsetjmp(arg->jerr.setjmp_buffer, 1)) {
		jpeg_destroy_decompress(&arg->jinfo);
		FREE(im, arg);
		im->load_arg = NULL;
		return;
	}

	jpeg_finish_decompress(&arg->jinfo);
	jpeg_destroy_decompress(&arg->jinfo);

	FREE(im, arg);
	im->load_arg = NULL;
	return;
}

void wi_jpeg_init_dest(j_compress_ptr jinfo)
{
	int ret;
	struct wi_jpeg_dst *dst;

	dst = (struct wi_jpeg_dst *)jinfo->dest;

	init_blob(&dst->im->ou_buf);
	ret = expand_blob(dst->im, &dst->im->ou_buf, MIN_OUTBUFF_LEN);
	if (ret != 0) ERREXIT(jinfo, JERR_FILE_WRITE);

	dst->pub.next_output_byte = (JOCTET *)dst->im->ou_buf.ptr;
	dst->pub.free_in_buffer = blob_left(&dst->im->ou_buf);
	return;
}

boolean wi_jpeg_write_buf(j_compress_ptr jinfo)
{
	int ret;
	struct blob *b;
	struct image *im;
	struct wi_jpeg_dst *dst;

	dst = (struct wi_jpeg_dst *)jinfo->dest;
	im = dst->im;
	b = &im->ou_buf;

	/* seek to the prev writed pos */
	b->ptr += MIN_OUTBUFF_LEN;

	/* fill buffer */
	ret = expand_blob(im, b, b->len + MIN_OUTBUFF_LEN);
	if (ret != 0) ERREXIT(jinfo, JERR_FILE_WRITE);

	dst->pub.next_output_byte = (JOCTET *)b->ptr;
	dst->pub.free_in_buffer = MIN_OUTBUFF_LEN;

	return (TRUE);
}

void wi_jpeg_term_dest(j_compress_ptr jinfo)
{
	struct wi_jpeg_dst *dst;

	dst = (struct wi_jpeg_dst *)jinfo->dest;
	dst->im->ou_buf.ptr += (MIN_OUTBUFF_LEN - dst->pub.free_in_buffer);
	dst->im->ou_buf.len = (dst->im->ou_buf.ptr - dst->im->ou_buf.data);
	return;
}

void wi_jpeg_blob_dest(j_compress_ptr jinfo, struct image *im)
{
	struct wi_jpeg_dst *dst;

	jinfo->dest = (struct jpeg_destination_mgr *) (*jinfo->mem->alloc_small)
		((j_common_ptr) jinfo, JPOOL_IMAGE, sizeof(struct wi_jpeg_dst));
	dst = (struct wi_jpeg_dst *)jinfo->dest;
	dst->pub.init_destination = wi_jpeg_init_dest;
	dst->pub.empty_output_buffer = wi_jpeg_write_buf;
	dst->pub.term_destination = wi_jpeg_term_dest;
	dst->im = im;
}

int wi_jpeg_save(struct image *im)
{
	int i, comps;
	struct wi_jpeg_error_mgr jerr;
	struct jpeg_compress_struct jinfo;

	comps = get_components(im);

	wi_jpeg_set_error_handler((j_common_ptr)&jinfo, &jerr);
	if (sigsetjmp(jerr.setjmp_buffer, 1)) {
		jpeg_destroy_compress(&jinfo);
		return -1;
	}
	/* exit via error handler if it failed */
	jpeg_create_compress(&jinfo);

	wi_jpeg_blob_dest(&jinfo, im);

	jinfo.image_width = im->cols;
	jinfo.image_height = im->rows;
	jinfo.input_components = comps;
	jinfo.in_color_space = im->colorspace == CS_RGB ? JCS_RGB : JCS_GRAYSCALE;
	jpeg_set_defaults(&jinfo);
	jpeg_default_colorspace(&jinfo);	/* RGB or GRAYSCALE */
	jpeg_set_quality(&jinfo, im->quality, TRUE);
	if (im->interlace) jpeg_simple_progression(&jinfo);
	//TODO
	jinfo.optimize_coding = TRUE;

	for (i=0; i<comps; i++) {
		jinfo.comp_info[i].component_id  = i;
		jinfo.comp_info[i].h_samp_factor = 1;
		jinfo.comp_info[i].v_samp_factor = 1;
	}

	/* start compress */
	jpeg_start_compress(&jinfo, TRUE);

	while (jinfo.next_scanline < jinfo.image_height) {
		jpeg_write_scanlines(&jinfo, (JSAMPROW *)im->row, im->rows);
	}

	jpeg_finish_compress(&jinfo);
	jpeg_destroy_compress(&jinfo);

	return 0;
}

struct loader jpeg_loader = {
	.name			= "JPEG",
	.ext			= ".jpg",
	.mime_type		= "image/jpeg",

	.suitable		= wi_is_jpeg,
	.init			= wi_jpeg_init_read,
	.read_info		= wi_jpeg_read_img_info,
	.resample		= wi_jpeg_resample,
	.load			= wi_jpeg_load_image,
	.finish			= wi_jpeg_finish_read,
	.save			= wi_jpeg_save,
};
