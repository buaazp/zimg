// Copyright 2013 Google Inc. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the COPYING file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS. All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.
// -----------------------------------------------------------------------------
//
//  Helper structs and methods for gif2webp tool.
//

#include <assert.h>
#include <stdio.h>

#include "webp/encode.h"
#include "./gif2webp_util.h"

#define DELTA_INFINITY      1ULL << 32
#define KEYFRAME_NONE       -1

//------------------------------------------------------------------------------
// Helper utilities.

static void ClearRectangle(WebPPicture* const picture,
                           int left, int top, int width, int height) {
  int j;
  for (j = top; j < top + height; ++j) {
    uint32_t* const dst = picture->argb + j * picture->argb_stride;
    int i;
    for (i = left; i < left + width; ++i) {
      dst[i] = WEBP_UTIL_TRANSPARENT_COLOR;
    }
  }
}

void WebPUtilClearPic(WebPPicture* const picture,
                      const WebPFrameRect* const rect) {
  if (rect != NULL) {
    ClearRectangle(picture, rect->x_offset, rect->y_offset,
                   rect->width, rect->height);
  } else {
    ClearRectangle(picture, 0, 0, picture->width, picture->height);
  }
}

// TODO: Also used in picture.c. Move to a common location?
// Copy width x height pixels from 'src' to 'dst' honoring the strides.
static void CopyPlane(const uint8_t* src, int src_stride,
                      uint8_t* dst, int dst_stride, int width, int height) {
  while (height-- > 0) {
    memcpy(dst, src, width);
    src += src_stride;
    dst += dst_stride;
  }
}

// Copy pixels from 'src' to 'dst' honoring strides. 'src' and 'dst' are assumed
// to be already allocated.
static void CopyPixels(const WebPPicture* const src, WebPPicture* const dst) {
  assert(src->width == dst->width && src->height == dst->height);
  CopyPlane((uint8_t*)src->argb, 4 * src->argb_stride, (uint8_t*)dst->argb,
            4 * dst->argb_stride, 4 * src->width, src->height);
}

// Given 'src' picture and its frame rectangle 'rect', blend it into 'dst'.
static void BlendPixels(const WebPPicture* const src,
                        const WebPFrameRect* const rect,
                        WebPPicture* const dst) {
  int j;
  assert(src->width == dst->width && src->height == dst->height);
  for (j = rect->y_offset; j < rect->y_offset + rect->height; ++j) {
    int i;
    for (i = rect->x_offset; i < rect->x_offset + rect->width; ++i) {
      const uint32_t src_pixel = src->argb[j * src->argb_stride + i];
      const int src_alpha = src_pixel >> 24;
      if (src_alpha != 0) {
        dst->argb[j * dst->argb_stride + i] = src_pixel;
      }
    }
  }
}

// Replace transparent pixels within 'dst_rect' of 'dst' by those in the 'src'.
static void ReduceTransparency(const WebPPicture* const src,
                               const WebPFrameRect* const rect,
                               WebPPicture* const dst) {
  int i, j;
  assert(src != NULL && dst != NULL && rect != NULL);
  assert(src->width == dst->width && src->height == dst->height);
  for (j = rect->y_offset; j < rect->y_offset + rect->height; ++j) {
    for (i = rect->x_offset; i < rect->x_offset + rect->width; ++i) {
      const uint32_t src_pixel = src->argb[j * src->argb_stride + i];
      const int src_alpha = src_pixel >> 24;
      const uint32_t dst_pixel = dst->argb[j * dst->argb_stride + i];
      const int dst_alpha = dst_pixel >> 24;
      if (dst_alpha == 0 && src_alpha == 0xff) {
        dst->argb[j * dst->argb_stride + i] = src_pixel;
      }
    }
  }
}

// Replace similar blocks of pixels by a 'see-through' transparent block
// with uniform average color.
static void FlattenSimilarBlocks(const WebPPicture* const src,
                                 const WebPFrameRect* const rect,
                                 WebPPicture* const dst) {
  int i, j;
  const int block_size = 8;
  const int y_start = (rect->y_offset + block_size) & ~(block_size - 1);
  const int y_end = (rect->y_offset + rect->height) & ~(block_size - 1);
  const int x_start = (rect->x_offset + block_size) & ~(block_size - 1);
  const int x_end = (rect->x_offset + rect->width) & ~(block_size - 1);
  assert(src != NULL && dst != NULL && rect != NULL);
  assert(src->width == dst->width && src->height == dst->height);
  assert((block_size & (block_size - 1)) == 0);  // must be a power of 2
  // Iterate over each block and count similar pixels.
  for (j = y_start; j < y_end; j += block_size) {
    for (i = x_start; i < x_end; i += block_size) {
      int cnt = 0;
      int avg_r = 0, avg_g = 0, avg_b = 0;
      int x, y;
      const uint32_t* const psrc = src->argb + j * src->argb_stride + i;
      uint32_t* const pdst = dst->argb + j * dst->argb_stride + i;
      for (y = 0; y < block_size; ++y) {
        for (x = 0; x < block_size; ++x) {
          const uint32_t src_pixel = psrc[x + y * src->argb_stride];
          const int alpha = src_pixel >> 24;
          if (alpha == 0xff &&
              src_pixel == pdst[x + y * dst->argb_stride]) {
              ++cnt;
              avg_r += (src_pixel >> 16) & 0xff;
              avg_g += (src_pixel >>  8) & 0xff;
              avg_b += (src_pixel >>  0) & 0xff;
          }
        }
      }
      // If we have a fully similar block, we replace it with an
      // average transparent block. This compresses better in lossy mode.
      if (cnt == block_size * block_size) {
        const uint32_t color = (0x00          << 24) |
                               ((avg_r / cnt) << 16) |
                               ((avg_g / cnt) <<  8) |
                               ((avg_b / cnt) <<  0);
        for (y = 0; y < block_size; ++y) {
          for (x = 0; x < block_size; ++x) {
            pdst[x + y * dst->argb_stride] = color;
          }
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
// Key frame related utilities.

// Returns true if 'curr' frame with frame rectangle 'curr_rect' is a key frame,
// that is, it can be decoded independently of 'prev' canvas.
static int IsKeyFrame(const WebPPicture* const curr,
                      const WebPFrameRect* const curr_rect,
                      const WebPPicture* const prev) {
  int i, j;
  int is_key_frame = 1;

  // If previous canvas (with previous frame disposed) is all transparent,
  // current frame is a key frame.
  for (j = 0; j < prev->height; ++j) {
    const uint32_t* const row = &prev->argb[j * prev->argb_stride];
    for (i = 0; i < prev->width; ++i) {
      if (row[i] & 0xff000000u) {   // has alpha?
        is_key_frame = 0;
        break;
      }
    }
    if (!is_key_frame) break;
  }
  if (is_key_frame) return 1;

  // If current frame covers the whole canvas and does not contain any
  // transparent pixels that depend on previous canvas, then current frame is
  // a key frame.
  if (curr_rect->width == curr->width && curr_rect->height == curr->height) {
    assert(curr_rect->x_offset == 0 && curr_rect->y_offset == 0);
    is_key_frame = 1;
    for (j = 0; j < prev->height; ++j) {
      for (i = 0; i < prev->width; ++i) {
        const uint32_t prev_alpha =
            (prev->argb[j * prev->argb_stride + i]) >> 24;
        const uint32_t curr_alpha =
            (curr->argb[j * curr->argb_stride + i]) >> 24;
        if (curr_alpha != 0xff && prev_alpha != 0) {
          is_key_frame = 0;
          break;
        }
      }
      if (!is_key_frame) break;
    }
    if (is_key_frame) return 1;
  }

  return 0;
}

// Given 'prev' frame and current frame rectangle 'rect', convert 'curr' frame
// to a key frame.
static void ConvertToKeyFrame(const WebPPicture* const prev,
                              WebPFrameRect* const rect,
                              WebPPicture* const curr) {
  int j;
  assert(curr->width == prev->width && curr->height == prev->height);

  // Replace transparent pixels of current canvas with those from previous
  // canvas (with previous frame disposed).
  for (j = 0; j < curr->height; ++j) {
    int i;
    for (i = 0; i < curr->width; ++i) {
      uint32_t* const curr_pixel = curr->argb + j * curr->argb_stride + i;
      const int curr_alpha = *curr_pixel >> 24;
      if (curr_alpha == 0) {
        *curr_pixel = prev->argb[j * prev->argb_stride + i];
      }
    }
  }

  // Frame rectangle now covers the whole canvas.
  rect->x_offset = 0;
  rect->y_offset = 0;
  rect->width = curr->width;
  rect->height = curr->height;
}

//------------------------------------------------------------------------------
// Encoded frame.

// Used to store two candidates of encoded data for an animation frame. One of
// the two will be chosen later.
typedef struct {
  WebPMuxFrameInfo sub_frame;  // Encoded frame rectangle.
  WebPMuxFrameInfo key_frame;  // Encoded frame if it was converted to keyframe.
} EncodedFrame;

// Release the data contained by 'encoded_frame'.
static void FrameRelease(EncodedFrame* const encoded_frame) {
  if (encoded_frame != NULL) {
    WebPDataClear(&encoded_frame->sub_frame.bitstream);
    WebPDataClear(&encoded_frame->key_frame.bitstream);
    memset(encoded_frame, 0, sizeof(*encoded_frame));
  }
}

//------------------------------------------------------------------------------
// Frame cache.

// Used to store encoded frames that haven't been output yet.
struct WebPFrameCache {
  EncodedFrame* encoded_frames;  // Array of encoded frames.
  size_t size;               // Number of allocated data elements.
  size_t start;              // Start index.
  size_t count;              // Number of valid data elements.
  int flush_count;           // If >0, ‘flush_count’ frames starting from
                             // 'start' are ready to be added to mux.
  int64_t best_delta;        // min(canvas size - frame size) over the frames.
                             // Can be negative in certain cases due to
                             // transparent pixels in a frame.
  int keyframe;              // Index of selected keyframe relative to 'start'.

  size_t kmin;                   // Min distance between key frames.
  size_t kmax;                   // Max distance between key frames.
  size_t count_since_key_frame;  // Frames seen since the last key frame.
  int allow_mixed;           // If true, each frame can be lossy or lossless.
  WebPPicture prev_canvas;   // Previous canvas (properly disposed).
  WebPPicture curr_canvas;   // Current canvas (temporary buffer).
  int is_first_frame;        // True if no frames have been added to the cache
                             // since WebPFrameCacheNew().
};

// Reset the counters in the cache struct. Doesn't touch 'cache->encoded_frames'
// and 'cache->size'.
static void CacheReset(WebPFrameCache* const cache) {
  cache->start = 0;
  cache->count = 0;
  cache->flush_count = 0;
  cache->best_delta = DELTA_INFINITY;
  cache->keyframe = KEYFRAME_NONE;
}

WebPFrameCache* WebPFrameCacheNew(int width, int height,
                                  size_t kmin, size_t kmax, int allow_mixed) {
  WebPFrameCache* cache = (WebPFrameCache*)malloc(sizeof(*cache));
  if (cache == NULL) return NULL;
  CacheReset(cache);
  // sanity init, so we can call WebPFrameCacheDelete():
  cache->encoded_frames = NULL;

  cache->is_first_frame = 1;

  // Picture buffers.
  if (!WebPPictureInit(&cache->prev_canvas) ||
      !WebPPictureInit(&cache->curr_canvas)) {
    return NULL;
  }
  cache->prev_canvas.width = width;
  cache->prev_canvas.height = height;
  cache->prev_canvas.use_argb = 1;
  if (!WebPPictureAlloc(&cache->prev_canvas) ||
      !WebPPictureCopy(&cache->prev_canvas, &cache->curr_canvas)) {
    goto Err;
  }
  WebPUtilClearPic(&cache->prev_canvas, NULL);

  // Cache data.
  cache->allow_mixed = allow_mixed;
  cache->kmin = kmin;
  cache->kmax = kmax;
  cache->count_since_key_frame = 0;
  assert(kmax > kmin);
  cache->size = kmax - kmin;
  cache->encoded_frames =
      (EncodedFrame*)calloc(cache->size, sizeof(*cache->encoded_frames));
  if (cache->encoded_frames == NULL) goto Err;

  return cache;  // All OK.

 Err:
  WebPFrameCacheDelete(cache);
  return NULL;
}

void WebPFrameCacheDelete(WebPFrameCache* const cache) {
  if (cache != NULL) {
    if (cache->encoded_frames != NULL) {
      size_t i;
      for (i = 0; i < cache->size; ++i) {
        FrameRelease(&cache->encoded_frames[i]);
      }
      free(cache->encoded_frames);
    }
    WebPPictureFree(&cache->prev_canvas);
    WebPPictureFree(&cache->curr_canvas);
    free(cache);
  }
}

static int EncodeFrame(const WebPConfig* const config, WebPPicture* const pic,
                       WebPMemoryWriter* const memory) {
  pic->use_argb = 1;
  pic->writer = WebPMemoryWrite;
  pic->custom_ptr = memory;
  if (!WebPEncode(config, pic)) {
    return 0;
  }
  return 1;
}

static void GetEncodedData(const WebPMemoryWriter* const memory,
                           WebPData* const encoded_data) {
  encoded_data->bytes = memory->mem;
  encoded_data->size  = memory->size;
}

#define MIN_COLORS_LOSSY     31  // Don't try lossy below this threshold.
#define MAX_COLORS_LOSSLESS 194  // Don't try lossless above this threshold.
#define MAX_COLOR_COUNT     256  // Power of 2 greater than MAX_COLORS_LOSSLESS.
#define HASH_SIZE (MAX_COLOR_COUNT * 4)
#define HASH_RIGHT_SHIFT     22  // 32 - log2(HASH_SIZE).

// TODO(urvang): Also used in enc/vp8l.c. Move to utils.
// If the number of colors in the 'pic' is at least MAX_COLOR_COUNT, return
// MAX_COLOR_COUNT. Otherwise, return the exact number of colors in the 'pic'.
static int GetColorCount(const WebPPicture* const pic) {
  int x, y;
  int num_colors = 0;
  uint8_t in_use[HASH_SIZE] = { 0 };
  uint32_t colors[HASH_SIZE];
  static const uint32_t kHashMul = 0x1e35a7bd;
  const uint32_t* argb = pic->argb;
  const int width = pic->width;
  const int height = pic->height;
  uint32_t last_pix = ~argb[0];   // so we're sure that last_pix != argb[0]

  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x) {
      int key;
      if (argb[x] == last_pix) {
        continue;
      }
      last_pix = argb[x];
      key = (kHashMul * last_pix) >> HASH_RIGHT_SHIFT;
      while (1) {
        if (!in_use[key]) {
          colors[key] = last_pix;
          in_use[key] = 1;
          ++num_colors;
          if (num_colors >= MAX_COLOR_COUNT) {
            return MAX_COLOR_COUNT;  // Exact count not needed.
          }
          break;
        } else if (colors[key] == last_pix) {
          break;  // The color is already there.
        } else {
          // Some other color sits here, so do linear conflict resolution.
          ++key;
          key &= (HASH_SIZE - 1);  // Key mask.
        }
      }
    }
    argb += pic->argb_stride;
  }
  return num_colors;
}

#undef MAX_COLOR_COUNT
#undef HASH_SIZE
#undef HASH_RIGHT_SHIFT

static WebPEncodingError SetFrame(const WebPConfig* const config,
                                  int allow_mixed, int is_key_frame,
                                  const WebPPicture* const prev_canvas,
                                  WebPPicture* const frame,
                                  const WebPFrameRect* const rect,
                                  const WebPMuxFrameInfo* const info,
                                  WebPPicture* const sub_frame,
                                  EncodedFrame* encoded_frame) {
  WebPEncodingError error_code = VP8_ENC_OK;
  int try_lossless;
  int try_lossy;
  int try_both;
  WebPMemoryWriter mem1, mem2;
  WebPData* encoded_data;
  WebPMuxFrameInfo* const dst =
      is_key_frame ? &encoded_frame->key_frame : &encoded_frame->sub_frame;
  *dst = *info;
  encoded_data = &dst->bitstream;
  WebPMemoryWriterInit(&mem1);
  WebPMemoryWriterInit(&mem2);

  if (!allow_mixed) {
    try_lossless = config->lossless;
    try_lossy = !try_lossless;
  } else {  // Use a heuristic for trying lossless and/or lossy compression.
    const int num_colors = GetColorCount(sub_frame);
    try_lossless = (num_colors < MAX_COLORS_LOSSLESS);
    try_lossy = (num_colors >= MIN_COLORS_LOSSY);
  }
  try_both = try_lossless && try_lossy;

  if (try_lossless) {
    WebPConfig config_ll = *config;
    config_ll.lossless = 1;
    if (!EncodeFrame(&config_ll, sub_frame, &mem1)) {
      error_code = sub_frame->error_code;
      goto Err;
    }
  }

  if (try_lossy) {
    WebPConfig config_lossy = *config;
    config_lossy.lossless = 0;
    if (!is_key_frame) {
      // For lossy compression of a frame, it's better to replace transparent
      // pixels of 'curr' with actual RGB values, whenever possible.
      ReduceTransparency(prev_canvas, rect, frame);
      // TODO(later): Investigate if this helps lossless compression as well.
      FlattenSimilarBlocks(prev_canvas, rect, frame);
    }
    if (!EncodeFrame(&config_lossy, sub_frame, &mem2)) {
      error_code = sub_frame->error_code;
      goto Err;
    }
  }

  if (try_both) {  // Pick the encoding with smallest size.
    // TODO(later): Perhaps a rough SSIM/PSNR produced by the encoder should
    // also be a criteria, in addition to sizes.
    if (mem1.size <= mem2.size) {
#if WEBP_ENCODER_ABI_VERSION > 0x0202
      WebPMemoryWriterClear(&mem2);
#else
      free(mem2.mem);
      memset(&mem2, 0, sizeof(mem2));
#endif
      GetEncodedData(&mem1, encoded_data);
    } else {
#if WEBP_ENCODER_ABI_VERSION > 0x0202
      WebPMemoryWriterClear(&mem1);
#else
      free(mem1.mem);
      memset(&mem1, 0, sizeof(mem1));
#endif
      GetEncodedData(&mem2, encoded_data);
    }
  } else {
    GetEncodedData(try_lossless ? &mem1 : &mem2, encoded_data);
  }
  return error_code;

 Err:
#if WEBP_ENCODER_ABI_VERSION > 0x0202
  WebPMemoryWriterClear(&mem1);
  WebPMemoryWriterClear(&mem2);
#else
  free(mem1.mem);
  free(mem2.mem);
#endif
  return error_code;
}

#undef MIN_COLORS_LOSSY
#undef MAX_COLORS_LOSSLESS

// Returns cached frame at given 'position' index.
static EncodedFrame* CacheGetFrame(const WebPFrameCache* const cache,
                                   size_t position) {
  assert(cache->start + position < cache->size);
  return &cache->encoded_frames[cache->start + position];
}

// Calculate the penalty incurred if we encode given frame as a key frame
// instead of a sub-frame.
static int64_t KeyFramePenalty(const EncodedFrame* const encoded_frame) {
  return ((int64_t)encoded_frame->key_frame.bitstream.size -
          encoded_frame->sub_frame.bitstream.size);
}

static void DisposeFrame(WebPMuxAnimDispose dispose_method,
                         const WebPFrameRect* const gif_rect,
                         WebPPicture* const frame, WebPPicture* const canvas) {
  if (dispose_method == WEBP_MUX_DISPOSE_BACKGROUND) {
    WebPUtilClearPic(frame, NULL);
    WebPUtilClearPic(canvas, gif_rect);
  }
}

int WebPFrameCacheAddFrame(WebPFrameCache* const cache,
                           const WebPConfig* const config,
                           const WebPFrameRect* const orig_rect_ptr,
                           WebPPicture* const frame,
                           WebPMuxFrameInfo* const info) {
  int ok = 0;
  WebPEncodingError error_code = VP8_ENC_OK;
  WebPFrameRect rect;
  WebPPicture sub_image;  // View extracted from 'frame' with rectangle 'rect'.
  WebPPicture* const prev_canvas = &cache->prev_canvas;
  const size_t position = cache->count;
  const int allow_mixed = cache->allow_mixed;
  EncodedFrame* const encoded_frame = CacheGetFrame(cache, position);
  WebPFrameRect orig_rect;
  assert(position < cache->size);

  if (frame == NULL || info == NULL) {
    return 0;
  }

  if (orig_rect_ptr == NULL) {
    orig_rect.width = frame->width;
    orig_rect.height = frame->height;
    orig_rect.x_offset = 0;
    orig_rect.y_offset = 0;
  } else {
    orig_rect = *orig_rect_ptr;
  }

  // Snap to even offsets (and adjust dimensions if needed).
  rect = orig_rect;
  rect.width += (rect.x_offset & 1);
  rect.height += (rect.y_offset & 1);
  rect.x_offset &= ~1;
  rect.y_offset &= ~1;

  if (!WebPPictureView(frame, rect.x_offset, rect.y_offset,
                       rect.width, rect.height, &sub_image)) {
    return 0;
  }
  info->x_offset = rect.x_offset;
  info->y_offset = rect.y_offset;

  ++cache->count;

  if (cache->is_first_frame || IsKeyFrame(frame, &rect, prev_canvas)) {
    // Add this as a key frame.
    error_code = SetFrame(config, allow_mixed, 1, NULL, NULL, NULL,
                          info, &sub_image, encoded_frame);
    if (error_code != VP8_ENC_OK) {
      goto End;
    }
    cache->keyframe = position;
    cache->flush_count = cache->count;
    cache->count_since_key_frame = 0;
    // Update prev_canvas by simply copying from 'curr'.
    CopyPixels(frame, prev_canvas);
  } else {
    ++cache->count_since_key_frame;
    if (cache->count_since_key_frame <= cache->kmin) {
      // Add this as a frame rectangle.
      error_code = SetFrame(config, allow_mixed, 0, prev_canvas, frame,
                            &rect, info, &sub_image, encoded_frame);
      if (error_code != VP8_ENC_OK) {
        goto End;
      }
      cache->flush_count = cache->count;
      // Update prev_canvas by blending 'curr' into it.
      BlendPixels(frame, &orig_rect, prev_canvas);
    } else {
      WebPPicture full_image;
      WebPMuxFrameInfo full_image_info;
      int64_t curr_delta;

      // Add frame rectangle to cache.
      error_code = SetFrame(config, allow_mixed, 0, prev_canvas, frame, &rect,
                            info, &sub_image, encoded_frame);
      if (error_code != VP8_ENC_OK) {
        goto End;
      }

      // Convert to a key frame.
      CopyPixels(frame, &cache->curr_canvas);
      ConvertToKeyFrame(prev_canvas, &rect, &cache->curr_canvas);
      if (!WebPPictureView(&cache->curr_canvas, rect.x_offset, rect.y_offset,
                           rect.width, rect.height, &full_image)) {
        goto End;
      }
      full_image_info = *info;
      full_image_info.x_offset = rect.x_offset;
      full_image_info.y_offset = rect.y_offset;

      // Add key frame to cache, too.
      error_code = SetFrame(config, allow_mixed, 1, NULL, NULL, NULL,
                            &full_image_info, &full_image, encoded_frame);
      WebPPictureFree(&full_image);
      if (error_code != VP8_ENC_OK) goto End;

      // Analyze size difference of the two variants.
      curr_delta = KeyFramePenalty(encoded_frame);
      if (curr_delta <= cache->best_delta) {  // Pick this as keyframe.
        cache->keyframe = position;
        cache->best_delta = curr_delta;
        cache->flush_count = cache->count - 1;  // We can flush previous frames.
      }
      if (cache->count_since_key_frame == cache->kmax) {
        cache->flush_count = cache->count;
        cache->count_since_key_frame = 0;
      }

      // Update prev_canvas by simply copying from 'curr_canvas'.
      CopyPixels(&cache->curr_canvas, prev_canvas);
    }
  }

  DisposeFrame(info->dispose_method, &orig_rect, frame, prev_canvas);

  cache->is_first_frame = 0;
  ok = 1;

 End:
  WebPPictureFree(&sub_image);
  if (!ok) {
    FrameRelease(encoded_frame);
    --cache->count;  // We reset the count, as the frame addition failed.
  }
  frame->error_code = error_code;   // report error_code
  assert(ok || error_code != VP8_ENC_OK);
  return ok;
}

WebPMuxError WebPFrameCacheFlush(WebPFrameCache* const cache, int verbose,
                                 WebPMux* const mux) {
  while (cache->flush_count > 0) {
    WebPMuxFrameInfo* info;
    WebPMuxError err;
    EncodedFrame* const curr = CacheGetFrame(cache, 0);
    // Pick frame or full canvas.
    if (cache->keyframe == 0) {
      info = &curr->key_frame;
      info->blend_method = WEBP_MUX_NO_BLEND;
      cache->keyframe = KEYFRAME_NONE;
      cache->best_delta = DELTA_INFINITY;
    } else {
      info = &curr->sub_frame;
      info->blend_method = WEBP_MUX_BLEND;
    }
    // Add to mux.
    err = WebPMuxPushFrame(mux, info, 1);
    if (err != WEBP_MUX_OK) return err;
    if (verbose) {
      printf("Added frame. offset:%d,%d duration:%d dispose:%d blend:%d\n",
             info->x_offset, info->y_offset, info->duration,
             info->dispose_method, info->blend_method);
    }
    FrameRelease(curr);
    ++cache->start;
    --cache->flush_count;
    --cache->count;
    if (cache->keyframe != KEYFRAME_NONE) --cache->keyframe;
  }

  if (cache->count == 0) CacheReset(cache);
  return WEBP_MUX_OK;
}

WebPMuxError WebPFrameCacheFlushAll(WebPFrameCache* const cache, int verbose,
                                    WebPMux* const mux) {
  cache->flush_count = cache->count;  // Force flushing of all frames.
  return WebPFrameCacheFlush(cache, verbose, mux);
}

//------------------------------------------------------------------------------
