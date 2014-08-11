// Copyright 2014 Google Inc. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the COPYING file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS. All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.
// -----------------------------------------------------------------------------
//
// WebPPicture utils for colorspace conversion
//
// Author: Skal (pascal.massimino@gmail.com)

#include <assert.h>
#include <stdlib.h>
#include <math.h>

#include "./vp8enci.h"
#include "../utils/random.h"
#include "../dsp/yuv.h"

// Uncomment to disable gamma-compression during RGB->U/V averaging
#define USE_GAMMA_COMPRESSION

static const union {
  uint32_t argb;
  uint8_t  bytes[4];
} test_endian = { 0xff000000u };
#define ALPHA_IS_LAST (test_endian.bytes[3] == 0xff)

static WEBP_INLINE uint32_t MakeARGB32(int a, int r, int g, int b) {
  return (((uint32_t)a << 24) | (r << 16) | (g << 8) | b);
}

//------------------------------------------------------------------------------
// Detection of non-trivial transparency

// Returns true if alpha[] has non-0xff values.
static int CheckNonOpaque(const uint8_t* alpha, int width, int height,
                          int x_step, int y_step) {
  if (alpha == NULL) return 0;
  while (height-- > 0) {
    int x;
    for (x = 0; x < width * x_step; x += x_step) {
      if (alpha[x] != 0xff) return 1;  // TODO(skal): check 4/8 bytes at a time.
    }
    alpha += y_step;
  }
  return 0;
}

// Checking for the presence of non-opaque alpha.
int WebPPictureHasTransparency(const WebPPicture* picture) {
  if (picture == NULL) return 0;
  if (!picture->use_argb) {
    return CheckNonOpaque(picture->a, picture->width, picture->height,
                          1, picture->a_stride);
  } else {
    int x, y;
    const uint32_t* argb = picture->argb;
    if (argb == NULL) return 0;
    for (y = 0; y < picture->height; ++y) {
      for (x = 0; x < picture->width; ++x) {
        if (argb[x] < 0xff000000u) return 1;   // test any alpha values != 0xff
      }
      argb += picture->argb_stride;
    }
  }
  return 0;
}

//------------------------------------------------------------------------------
// RGB -> YUV conversion

static int RGBToY(int r, int g, int b, VP8Random* const rg) {
  return VP8RGBToY(r, g, b, VP8RandomBits(rg, YUV_FIX));
}

static int RGBToU(int r, int g, int b, VP8Random* const rg) {
  return VP8RGBToU(r, g, b, VP8RandomBits(rg, YUV_FIX + 2));
}

static int RGBToV(int r, int g, int b, VP8Random* const rg) {
  return VP8RGBToV(r, g, b, VP8RandomBits(rg, YUV_FIX + 2));
}

//------------------------------------------------------------------------------

#if defined(USE_GAMMA_COMPRESSION)

// gamma-compensates loss of resolution during chroma subsampling
#define kGamma 0.80
#define kGammaFix 12     // fixed-point precision for linear values
#define kGammaScale ((1 << kGammaFix) - 1)
#define kGammaTabFix 7   // fixed-point fractional bits precision
#define kGammaTabScale (1 << kGammaTabFix)
#define kGammaTabRounder (kGammaTabScale >> 1)
#define kGammaTabSize (1 << (kGammaFix - kGammaTabFix))

static int kLinearToGammaTab[kGammaTabSize + 1];
static uint16_t kGammaToLinearTab[256];
static int kGammaTablesOk = 0;

static void InitGammaTables(void) {
  if (!kGammaTablesOk) {
    int v;
    const double scale = 1. / kGammaScale;
    for (v = 0; v <= 255; ++v) {
      kGammaToLinearTab[v] =
          (uint16_t)(pow(v / 255., kGamma) * kGammaScale + .5);
    }
    for (v = 0; v <= kGammaTabSize; ++v) {
      const double x = scale * (v << kGammaTabFix);
      kLinearToGammaTab[v] = (int)(pow(x, 1. / kGamma) * 255. + .5);
    }
    kGammaTablesOk = 1;
  }
}

static WEBP_INLINE uint32_t GammaToLinear(uint8_t v) {
  return kGammaToLinearTab[v];
}

// Convert a linear value 'v' to YUV_FIX+2 fixed-point precision
// U/V value, suitable for RGBToU/V calls.
static WEBP_INLINE int LinearToGamma(uint32_t base_value, int shift) {
  const int v = base_value << shift;              // final uplifted value
  const int tab_pos = v >> (kGammaTabFix + 2);    // integer part
  const int x = v & ((kGammaTabScale << 2) - 1);  // fractional part
  const int v0 = kLinearToGammaTab[tab_pos];
  const int v1 = kLinearToGammaTab[tab_pos + 1];
  const int y = v1 * x + v0 * ((kGammaTabScale << 2) - x);   // interpolate
  return (y + kGammaTabRounder) >> kGammaTabFix;             // descale
}

#else

static void InitGammaTables(void) {}
static WEBP_INLINE uint32_t GammaToLinear(uint8_t v) { return v; }
static WEBP_INLINE int LinearToGamma(uint32_t base_value, int shift) {
  return (int)(base_value << shift);
}

#endif    // USE_GAMMA_COMPRESSION

//------------------------------------------------------------------------------

#define SUM4(ptr) LinearToGamma(                         \
    GammaToLinear((ptr)[0]) +                            \
    GammaToLinear((ptr)[step]) +                         \
    GammaToLinear((ptr)[rgb_stride]) +                   \
    GammaToLinear((ptr)[rgb_stride + step]), 0)          \

#define SUM2H(ptr) \
    LinearToGamma(GammaToLinear((ptr)[0]) + GammaToLinear((ptr)[step]), 1)
#define SUM2V(ptr) \
    LinearToGamma(GammaToLinear((ptr)[0]) + GammaToLinear((ptr)[rgb_stride]), 1)
#define SUM1(ptr)  \
    LinearToGamma(GammaToLinear((ptr)[0]), 2)

#define RGB_TO_UV(x, y, SUM) {                           \
  const int src = (2 * (step * (x) + (y) * rgb_stride)); \
  const int dst = (x) + (y) * picture->uv_stride;        \
  const int r = SUM(r_ptr + src);                        \
  const int g = SUM(g_ptr + src);                        \
  const int b = SUM(b_ptr + src);                        \
  picture->u[dst] = RGBToU(r, g, b, &rg);                \
  picture->v[dst] = RGBToV(r, g, b, &rg);                \
}

static int ImportYUVAFromRGBA(const uint8_t* const r_ptr,
                              const uint8_t* const g_ptr,
                              const uint8_t* const b_ptr,
                              const uint8_t* const a_ptr,
                              int step,         // bytes per pixel
                              int rgb_stride,   // bytes per scanline
                              float dithering,
                              WebPPicture* const picture) {
  int x, y;
  const int width = picture->width;
  const int height = picture->height;
  const int has_alpha = CheckNonOpaque(a_ptr, width, height, step, rgb_stride);
  VP8Random rg;

  if (has_alpha) {
    picture->colorspace |= WEBP_CSP_ALPHA_BIT;
  } else {
    picture->colorspace &= WEBP_CSP_UV_MASK;
  }
  picture->use_argb = 0;

  if (!WebPPictureAllocYUVA(picture, width, height)) return 0;

  VP8InitRandom(&rg, dithering);
  InitGammaTables();

  // Import luma plane
  for (y = 0; y < height; ++y) {
    uint8_t* const dst = &picture->y[y * picture->y_stride];
    for (x = 0; x < width; ++x) {
      const int offset = step * x + y * rgb_stride;
      dst[x] = RGBToY(r_ptr[offset], g_ptr[offset], b_ptr[offset], &rg);
    }
  }

  // Downsample U/V plane
  for (y = 0; y < (height >> 1); ++y) {
    for (x = 0; x < (width >> 1); ++x) {
      RGB_TO_UV(x, y, SUM4);
    }
    if (width & 1) {
      RGB_TO_UV(x, y, SUM2V);
    }
  }
  if (height & 1) {
    for (x = 0; x < (width >> 1); ++x) {
      RGB_TO_UV(x, y, SUM2H);
    }
    if (width & 1) {
      RGB_TO_UV(x, y, SUM1);
    }
  }

  if (has_alpha) {
    assert(step >= 4);
    assert(picture->a != NULL);
    for (y = 0; y < height; ++y) {
      for (x = 0; x < width; ++x) {
        picture->a[x + y * picture->a_stride] =
            a_ptr[step * x + y * rgb_stride];
      }
    }
  }
  return 1;
}

#undef SUM4
#undef SUM2V
#undef SUM2H
#undef SUM1
#undef RGB_TO_UV

//------------------------------------------------------------------------------
// call for ARGB->YUVA conversion

int WebPPictureARGBToYUVADithered(WebPPicture* picture, WebPEncCSP colorspace,
                                  float dithering) {
  if (picture == NULL) return 0;
  if (picture->argb == NULL) {
    return WebPEncodingSetError(picture, VP8_ENC_ERROR_NULL_PARAMETER);
  } else {
    const uint8_t* const argb = (const uint8_t*)picture->argb;
    const uint8_t* const r = ALPHA_IS_LAST ? argb + 2 : argb + 1;
    const uint8_t* const g = ALPHA_IS_LAST ? argb + 1 : argb + 2;
    const uint8_t* const b = ALPHA_IS_LAST ? argb + 0 : argb + 3;
    const uint8_t* const a = ALPHA_IS_LAST ? argb + 3 : argb + 0;

    picture->colorspace = colorspace;
    return ImportYUVAFromRGBA(r, g, b, a, 4, 4 * picture->argb_stride,
                              dithering, picture);
  }
}

int WebPPictureARGBToYUVA(WebPPicture* picture, WebPEncCSP colorspace) {
  return WebPPictureARGBToYUVADithered(picture, colorspace, 0.f);
}

//------------------------------------------------------------------------------
// call for YUVA -> ARGB conversion

int WebPPictureYUVAToARGB(WebPPicture* picture) {
  if (picture == NULL) return 0;
  if (picture->y == NULL || picture->u == NULL || picture->v == NULL) {
    return WebPEncodingSetError(picture, VP8_ENC_ERROR_NULL_PARAMETER);
  }
  if ((picture->colorspace & WEBP_CSP_ALPHA_BIT) && picture->a == NULL) {
    return WebPEncodingSetError(picture, VP8_ENC_ERROR_NULL_PARAMETER);
  }
  if ((picture->colorspace & WEBP_CSP_UV_MASK) != WEBP_YUV420) {
    return WebPEncodingSetError(picture, VP8_ENC_ERROR_INVALID_CONFIGURATION);
  }
  // Allocate a new argb buffer (discarding the previous one).
  if (!WebPPictureAllocARGB(picture, picture->width, picture->height)) return 0;
  picture->use_argb = 1;

  // Convert
  {
    int y;
    const int width = picture->width;
    const int height = picture->height;
    const int argb_stride = 4 * picture->argb_stride;
    uint8_t* dst = (uint8_t*)picture->argb;
    const uint8_t *cur_u = picture->u, *cur_v = picture->v, *cur_y = picture->y;
    WebPUpsampleLinePairFunc upsample = WebPGetLinePairConverter(ALPHA_IS_LAST);

    // First row, with replicated top samples.
    upsample(cur_y, NULL, cur_u, cur_v, cur_u, cur_v, dst, NULL, width);
    cur_y += picture->y_stride;
    dst += argb_stride;
    // Center rows.
    for (y = 1; y + 1 < height; y += 2) {
      const uint8_t* const top_u = cur_u;
      const uint8_t* const top_v = cur_v;
      cur_u += picture->uv_stride;
      cur_v += picture->uv_stride;
      upsample(cur_y, cur_y + picture->y_stride, top_u, top_v, cur_u, cur_v,
               dst, dst + argb_stride, width);
      cur_y += 2 * picture->y_stride;
      dst += 2 * argb_stride;
    }
    // Last row (if needed), with replicated bottom samples.
    if (height > 1 && !(height & 1)) {
      upsample(cur_y, NULL, cur_u, cur_v, cur_u, cur_v, dst, NULL, width);
    }
    // Insert alpha values if needed, in replacement for the default 0xff ones.
    if (picture->colorspace & WEBP_CSP_ALPHA_BIT) {
      for (y = 0; y < height; ++y) {
        uint32_t* const argb_dst = picture->argb + y * picture->argb_stride;
        const uint8_t* const src = picture->a + y * picture->a_stride;
        int x;
        for (x = 0; x < width; ++x) {
          argb_dst[x] = (argb_dst[x] & 0x00ffffffu) | ((uint32_t)src[x] << 24);
        }
      }
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
// automatic import / conversion

static int Import(WebPPicture* const picture,
                  const uint8_t* const rgb, int rgb_stride,
                  int step, int swap_rb, int import_alpha) {
  int y;
  const uint8_t* const r_ptr = rgb + (swap_rb ? 2 : 0);
  const uint8_t* const g_ptr = rgb + 1;
  const uint8_t* const b_ptr = rgb + (swap_rb ? 0 : 2);
  const uint8_t* const a_ptr = import_alpha ? rgb + 3 : NULL;
  const int width = picture->width;
  const int height = picture->height;

  if (!picture->use_argb) {
    return ImportYUVAFromRGBA(r_ptr, g_ptr, b_ptr, a_ptr, step, rgb_stride,
                              0.f /* no dithering */, picture);
  }
  if (!WebPPictureAlloc(picture)) return 0;

  assert(step >= (import_alpha ? 4 : 3));
  for (y = 0; y < height; ++y) {
    uint32_t* const dst = &picture->argb[y * picture->argb_stride];
    int x;
    for (x = 0; x < width; ++x) {
      const int offset = step * x + y * rgb_stride;
      dst[x] = MakeARGB32(import_alpha ? a_ptr[offset] : 0xff,
                          r_ptr[offset], g_ptr[offset], b_ptr[offset]);
    }
  }
  return 1;
}

// Public API

int WebPPictureImportRGB(WebPPicture* picture,
                         const uint8_t* rgb, int rgb_stride) {
  return (picture != NULL) ? Import(picture, rgb, rgb_stride, 3, 0, 0) : 0;
}

int WebPPictureImportBGR(WebPPicture* picture,
                         const uint8_t* rgb, int rgb_stride) {
  return (picture != NULL) ? Import(picture, rgb, rgb_stride, 3, 1, 0) : 0;
}

int WebPPictureImportRGBA(WebPPicture* picture,
                          const uint8_t* rgba, int rgba_stride) {
  return (picture != NULL) ? Import(picture, rgba, rgba_stride, 4, 0, 1) : 0;
}

int WebPPictureImportBGRA(WebPPicture* picture,
                          const uint8_t* rgba, int rgba_stride) {
  return (picture != NULL) ? Import(picture, rgba, rgba_stride, 4, 1, 1) : 0;
}

int WebPPictureImportRGBX(WebPPicture* picture,
                          const uint8_t* rgba, int rgba_stride) {
  return (picture != NULL) ? Import(picture, rgba, rgba_stride, 4, 0, 0) : 0;
}

int WebPPictureImportBGRX(WebPPicture* picture,
                          const uint8_t* rgba, int rgba_stride) {
  return (picture != NULL) ? Import(picture, rgba, rgba_stride, 4, 1, 0) : 0;
}

//------------------------------------------------------------------------------
