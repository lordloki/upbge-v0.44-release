/* SPDX-FileCopyrightText: 2015 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup imbuf
 */

#include <algorithm>
#include <cstddef>

#include "IMB_imbuf.hh"
#include "IMB_imbuf_types.hh"

#include "BLI_math_vector.h"
#include "BLI_utildefines.h"

#include "DNA_scene_types.h"

/* prototypes */
struct Stereo3DData;
static void imb_stereo3d_write_doit(Stereo3DData *s3d_data, const Stereo3dFormat *s3d);
static void imb_stereo3d_read_doit(Stereo3DData *s3d_data, const Stereo3dFormat *s3d);

struct Stereo3DData {
  struct {
    float *left, *right, *stereo;
  } rectf;
  struct {
    uchar *left, *right, *stereo;
  } rect;
  size_t x, y, channels;
  bool is_float;
};

/* -------------------------------------------------------------------- */
/** \name Local Functions
 * \{ */

static void imb_stereo3d_write_anaglyph(const Stereo3DData *s3d, enum eStereo3dAnaglyphType mode)
{
  int x, y;
  size_t width = s3d->x;
  size_t height = s3d->y;
  const size_t channels = s3d->channels;

  const int stride_from = width;
  const int stride_to = width;

  const int anaglyph_encoding[3][3] = {
      {0, 1, 1},
      {1, 0, 1},
      {0, 0, 1},
  };

  int r, g, b;

  r = anaglyph_encoding[mode][0];
  g = anaglyph_encoding[mode][1];
  b = anaglyph_encoding[mode][2];

  if (s3d->is_float) {
    float *rect_left = s3d->rectf.left;
    float *rect_right = s3d->rectf.right;
    float *rect_to = s3d->rectf.stereo;

    if (channels == 3) {
      for (y = 0; y < height; y++) {
        float *to = rect_to + stride_to * y * 3;
        float *from[2] = {
            rect_left + stride_from * y * 3,
            rect_right + stride_from * y * 3,
        };

        for (x = 0; x < width; x++, from[0] += 3, from[1] += 3, to += 3) {
          to[0] = from[r][0];
          to[1] = from[g][1];
          to[2] = from[b][2];
        }
      }
    }
    else if (channels == 4) {
      for (y = 0; y < height; y++) {
        float *to = rect_to + stride_to * y * 4;
        float *from[2] = {
            rect_left + stride_from * y * 4,
            rect_right + stride_from * y * 4,
        };

        for (x = 0; x < width; x++, from[0] += 4, from[1] += 4, to += 4) {
          to[0] = from[r][0];
          to[1] = from[g][1];
          to[2] = from[b][2];
          to[3] = std::max(from[0][3], from[1][3]);
        }
      }
    }
  }
  else {
    uchar *rect_left = s3d->rect.left;
    uchar *rect_right = s3d->rect.right;
    uchar *rect_to = s3d->rect.stereo;

    if (channels == 3) {
      for (y = 0; y < height; y++) {
        uchar *to = rect_to + stride_to * y * 3;
        uchar *from[2] = {
            rect_left + stride_from * y * 3,
            rect_right + stride_from * y * 3,
        };

        for (x = 0; x < width; x++, from[0] += 3, from[1] += 3, to += 3) {
          to[0] = from[r][0];
          to[1] = from[g][1];
          to[2] = from[b][2];
        }
      }
    }
    else if (channels == 4) {
      for (y = 0; y < height; y++) {
        uchar *to = rect_to + stride_to * y * 4;
        uchar *from[2] = {
            rect_left + stride_from * y * 4,
            rect_right + stride_from * y * 4,
        };

        for (x = 0; x < width; x++, from[0] += 4, from[1] += 4, to += 4) {
          to[0] = from[r][0];
          to[1] = from[g][1];
          to[2] = from[b][2];
          to[3] = std::max(from[0][3], from[1][3]);
        }
      }
    }
  }
}

static void imb_stereo3d_write_interlace(const Stereo3DData *s3d,
                                         enum eStereo3dInterlaceType mode,
                                         const bool swap)
{
  int x, y;
  size_t width = s3d->x;
  size_t height = s3d->y;
  const size_t channels = s3d->channels;

  const int stride_from = width;
  const int stride_to = width;

  if (s3d->is_float) {
    const float *rect_left = s3d->rectf.left;
    const float *rect_right = s3d->rectf.right;
    float *rect_to = s3d->rectf.stereo;

    switch (mode) {
      case S3D_INTERLACE_ROW: {
        char i = char(swap);
        for (y = 0; y < height; y++) {
          float *to = rect_to + stride_to * y * channels;
          const float *from[2] = {
              rect_left + stride_from * y * channels,
              rect_right + stride_from * y * channels,
          };
          memcpy(to, from[i], sizeof(float) * channels * stride_from);
          i = !i;
        }
        break;
      }
      case S3D_INTERLACE_COLUMN: {
        if (channels == 1) {
          for (y = 0; y < height; y++) {
            float *to = rect_to + stride_to * y;
            const float *from[2] = {
                rect_left + stride_from * y,
                rect_right + stride_from * y,
            };

            char i = char(swap);
            for (x = 0; x < width; x++, from[0] += 1, from[1] += 1, to += 1) {
              to[0] = from[i][0];
              i = !i;
            }
          }
        }
        else if (channels == 3) {
          for (y = 0; y < height; y++) {
            float *to = rect_to + stride_to * y * 3;
            const float *from[2] = {
                rect_left + stride_from * y * 3,
                rect_right + stride_from * y * 3,
            };

            char i = char(swap);
            for (x = 0; x < width; x++, from[0] += 3, from[1] += 3, to += 3) {
              copy_v3_v3(to, from[i]);
              i = !i;
            }
          }
        }
        else if (channels == 4) {
          for (y = 0; y < height; y++) {
            float *to = rect_to + stride_to * y * channels;
            const float *from[2] = {
                rect_left + stride_from * y * channels,
                rect_right + stride_from * y * channels,
            };

            char i = char(swap);
            for (x = 0; x < width; x++, from[0] += 4, from[1] += 4, to += 4) {
              copy_v4_v4(to, from[i]);
              i = !i;
            }
          }
        }
        break;
      }
      case S3D_INTERLACE_CHECKERBOARD: {
        if (channels == 1) {
          char i = char(swap);
          for (y = 0; y < height; y++) {
            float *to = rect_to + stride_to * y;
            const float *from[2] = {
                rect_left + stride_from * y,
                rect_right + stride_from * y,
            };
            char j = i;
            for (x = 0; x < width; x++, from[0] += 1, from[1] += 1, to += 1) {
              to[0] = from[j][0];
              j = !j;
            }
            i = !i;
          }
        }
        else if (channels == 3) {
          char i = char(swap);
          for (y = 0; y < height; y++) {
            float *to = rect_to + stride_to * y * 3;
            const float *from[2] = {
                rect_left + stride_from * y * 3,
                rect_right + stride_from * y * 3,
            };
            char j = i;
            for (x = 0; x < width; x++, from[0] += 3, from[1] += 3, to += 3) {
              copy_v3_v3(to, from[j]);
              j = !j;
            }
            i = !i;
          }
        }
        else if (channels == 4) {
          char i = char(swap);
          for (y = 0; y < height; y++) {
            float *to = rect_to + stride_to * y * 4;
            const float *from[2] = {
                rect_left + stride_from * y * 4,
                rect_right + stride_from * y * 4,
            };
            char j = i;
            for (x = 0; x < width; x++, from[0] += 4, from[1] += 4, to += 4) {
              copy_v4_v4(to, from[j]);
              j = !j;
            }
            i = !i;
          }
        }
        break;
      }
      default: {
        break;
      }
    }
  }
  else {
    const uchar *rect_left = s3d->rect.left;
    const uchar *rect_right = s3d->rect.right;
    uchar *rect_to = s3d->rect.stereo;

    switch (mode) {
      case S3D_INTERLACE_ROW: {
        char i = char(swap);
        for (y = 0; y < height; y++) {
          uchar *to = rect_to + stride_to * y * channels;
          const uchar *from[2] = {
              rect_left + stride_from * y * channels,
              rect_right + stride_from * y * channels,
          };
          memcpy(to, from[i], sizeof(uchar) * channels * stride_from);
          i = !i;
        }
        break;
      }
      case S3D_INTERLACE_COLUMN: {
        if (channels == 1) {
          for (y = 0; y < height; y++) {
            uchar *to = rect_to + stride_to * y;
            const uchar *from[2] = {
                rect_left + stride_from * y,
                rect_right + stride_from * y,
            };
            char i = char(swap);
            for (x = 0; x < width; x++, from[0] += 1, from[1] += 1, to += 1) {
              to[0] = from[i][0];
              i = !i;
            }
          }
        }
        else if (channels == 3) {
          for (y = 0; y < height; y++) {
            uchar *to = rect_to + stride_to * y * 3;
            const uchar *from[2] = {
                rect_left + stride_from * y * 3,
                rect_right + stride_from * y * 3,
            };
            char i = char(swap);
            for (x = 0; x < width; x++, from[0] += 3, from[1] += 3, to += 3) {
              copy_v3_v3_uchar(to, from[i]);
              i = !i;
            }
          }
        }
        else if (channels == 4) {
          for (y = 0; y < height; y++) {
            uchar *to = rect_to + stride_to * y * 4;
            const uchar *from[2] = {
                rect_left + stride_from * y * 4,
                rect_right + stride_from * y * 4,
            };
            char i = char(swap);
            for (x = 0; x < width; x++, from[0] += 4, from[1] += 4, to += 4) {
              copy_v4_v4_uchar(to, from[i]);
              i = !i;
            }
          }
        }
        break;
      }
      case S3D_INTERLACE_CHECKERBOARD: {
        if (channels == 1) {
          char i = char(swap);
          for (y = 0; y < height; y++) {
            uchar *to = rect_to + stride_to * y;
            const uchar *from[2] = {
                rect_left + stride_from * y,
                rect_right + stride_from * y,
            };
            char j = i;
            for (x = 0; x < width; x++, from[0] += 1, from[1] += 1, to += 1) {
              to[0] = from[j][0];
              j = !j;
            }
            i = !i;
          }
        }
        else if (channels == 3) {
          char i = char(swap);
          for (y = 0; y < height; y++) {
            uchar *to = rect_to + stride_to * y * 3;
            const uchar *from[2] = {
                rect_left + stride_from * y * 3,
                rect_right + stride_from * y * 3,
            };
            char j = i;
            for (x = 0; x < width; x++, from[0] += 3, from[1] += 3, to += 3) {
              copy_v3_v3_uchar(to, from[j]);
              j = !j;
            }
            i = !i;
          }
        }
        else if (channels == 4) {
          char i = char(swap);
          for (y = 0; y < height; y++) {
            uchar *to = rect_to + stride_to * y * 4;
            const uchar *from[2] = {
                rect_left + stride_from * y * 4,
                rect_right + stride_from * y * 4,
            };
            char j = i;
            for (x = 0; x < width; x++, from[0] += 4, from[1] += 4, to += 4) {
              copy_v4_v4_uchar(to, from[j]);
              j = !j;
            }
            i = !i;
          }
        }
        break;
      }
      default: {
        break;
      }
    }
  }
}

/* stereo3d output (s3d->rectf.stereo) is always unsqueezed */
static void imb_stereo3d_write_sidebyside(const Stereo3DData *s3d, const bool crosseyed)
{
  int y;
  size_t width = s3d->x;
  size_t height = s3d->y;
  const size_t channels = s3d->channels;

  const int stride_from = width;
  const int stride_to = width * 2;

  const int l = int(crosseyed);
  const int r = !l;

  if (s3d->is_float) {
    const float *rect_left = s3d->rectf.left;
    const float *rect_right = s3d->rectf.right;
    float *rect_to = s3d->rectf.stereo;

    for (y = 0; y < height; y++) {
      float *to = rect_to + stride_to * y * channels;
      const float *from[2] = {
          rect_left + stride_from * y * channels,
          rect_right + stride_from * y * channels,
      };

      memcpy(to, from[l], sizeof(float) * channels * stride_from);
      memcpy(to + channels * stride_from, from[r], sizeof(float) * channels * stride_from);
    }
  }
  else {
    const uchar *rect_left = s3d->rect.left;
    const uchar *rect_right = s3d->rect.right;
    uchar *rect_to = s3d->rect.stereo;

    for (y = 0; y < height; y++) {
      uchar *to = rect_to + stride_to * y * channels;
      const uchar *from[2] = {
          rect_left + stride_from * y * channels,
          rect_right + stride_from * y * channels,
      };

      memcpy(to, from[l], sizeof(uchar) * channels * stride_from);
      memcpy(to + channels * stride_from, from[r], sizeof(uchar) * channels * stride_from);
    }
  }
}

/* stereo3d output (s3d->rectf.stereo) is always unsqueezed */
static void imb_stereo3d_write_topbottom(const Stereo3DData *s3d)
{
  int y;
  size_t width = s3d->x;
  size_t height = s3d->y;
  const size_t channels = s3d->channels;

  const int stride_from = width;
  const int stride_to = width;

  if (s3d->is_float) {
    const float *rect_left = s3d->rectf.left;
    const float *rect_right = s3d->rectf.right;
    float *rect_to = s3d->rectf.stereo;

    for (y = 0; y < height; y++) {
      float *to = rect_to + stride_to * y * channels;
      const float *from[2] = {
          rect_left + stride_from * y * channels,
          rect_right + stride_from * y * channels,
      };

      memcpy(to, from[1], sizeof(float) * channels * stride_from);
      memcpy(
          to + channels * height * stride_from, from[0], sizeof(float) * channels * stride_from);
    }
  }
  else {
    const uchar *rect_left = s3d->rect.left;
    const uchar *rect_right = s3d->rect.right;
    uchar *rect_to = s3d->rect.stereo;

    for (y = 0; y < height; y++) {
      uchar *to = rect_to + stride_to * y * channels;
      const uchar *from[2] = {
          rect_left + stride_from * y * channels,
          rect_right + stride_from * y * channels,
      };

      memcpy(to, from[1], sizeof(uchar) * channels * stride_from);
      memcpy(
          to + channels * height * stride_from, from[0], sizeof(uchar) * channels * stride_from);
    }
  }
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Dimension Utils
 * \{ */

void IMB_stereo3d_write_dimensions(const char mode,
                                   const bool is_squeezed,
                                   const size_t width,
                                   const size_t height,
                                   size_t *r_width,
                                   size_t *r_height)
{
  switch (mode) {
    case S3D_DISPLAY_SIDEBYSIDE: {
      *r_width = is_squeezed ? width : width * 2;
      *r_height = height;
      break;
    }
    case S3D_DISPLAY_TOPBOTTOM: {
      *r_width = width;
      *r_height = is_squeezed ? height : height * 2;
      break;
    }
    case S3D_DISPLAY_ANAGLYPH:
    case S3D_DISPLAY_INTERLACE:
    default: {
      *r_width = width;
      *r_height = height;
      break;
    }
  }
}

void IMB_stereo3d_read_dimensions(const char mode,
                                  const bool is_squeezed,
                                  const size_t width,
                                  const size_t height,
                                  size_t *r_width,
                                  size_t *r_height)
{
  switch (mode) {
    case S3D_DISPLAY_SIDEBYSIDE: {
      *r_width = is_squeezed ? width / 2 : width;
      *r_height = height;
      break;
    }
    case S3D_DISPLAY_TOPBOTTOM: {
      *r_width = width;
      *r_height = is_squeezed ? height / 2 : height;
      break;
    }
    case S3D_DISPLAY_ANAGLYPH:
    case S3D_DISPLAY_INTERLACE:
    default: {
      *r_width = width;
      *r_height = height;
      break;
    }
  }
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Un/Squeeze Frame
 * \{ */

static void imb_stereo3d_squeeze_ImBuf(ImBuf *ibuf,
                                       const Stereo3dFormat *s3d,
                                       const size_t x,
                                       const size_t y)
{
  if (ELEM(s3d->display_mode, S3D_DISPLAY_SIDEBYSIDE, S3D_DISPLAY_TOPBOTTOM) == false) {
    return;
  }

  if ((s3d->flag & S3D_SQUEEZED_FRAME) == 0) {
    return;
  }

  IMB_scale(ibuf, x, y, IMBScaleFilter::Bilinear);
}

static void imb_stereo3d_unsqueeze_ImBuf(ImBuf *ibuf,
                                         const Stereo3dFormat *s3d,
                                         const size_t x,
                                         const size_t y)
{
  if (ELEM(s3d->display_mode, S3D_DISPLAY_SIDEBYSIDE, S3D_DISPLAY_TOPBOTTOM) == false) {
    return;
  }

  if ((s3d->flag & S3D_SQUEEZED_FRAME) == 0) {
    return;
  }

  IMB_scale(ibuf, x, y, IMBScaleFilter::Bilinear);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Preparing To Call The Write Functions
 * \{ */

static void imb_stereo3d_data_init(Stereo3DData *s3d_data,
                                   const bool is_float,
                                   const size_t x,
                                   const size_t y,
                                   const size_t channels,
                                   int *rect_left,
                                   int *rect_right,
                                   int *rect_stereo,
                                   float *rectf_left,
                                   float *rectf_right,
                                   float *rectf_stereo)
{
  s3d_data->is_float = is_float;
  s3d_data->x = x;
  s3d_data->y = y;
  s3d_data->channels = channels;
  s3d_data->rect.left = (uchar *)rect_left;
  s3d_data->rect.right = (uchar *)rect_right;
  s3d_data->rect.stereo = (uchar *)rect_stereo;
  s3d_data->rectf.left = rectf_left;
  s3d_data->rectf.right = rectf_right;
  s3d_data->rectf.stereo = rectf_stereo;
}

ImBuf *IMB_stereo3d_ImBuf(const ImageFormatData *im_format, ImBuf *ibuf_left, ImBuf *ibuf_right)
{
  ImBuf *ibuf_stereo = nullptr;
  Stereo3DData s3d_data = {{nullptr}};
  size_t width, height;
  const bool is_byte = ibuf_left->byte_buffer.data && ibuf_right->byte_buffer.data;
  const bool is_float = ibuf_left->float_buffer.data && ibuf_right->float_buffer.data &&
                        !(is_byte && im_format->depth <= 8);

  if (!(is_float || is_byte)) {
    return nullptr;
  }

  IMB_stereo3d_write_dimensions(
      im_format->stereo3d_format.display_mode, false, ibuf_left->x, ibuf_left->y, &width, &height);
  ibuf_stereo = IMB_allocImBuf(width, height, ibuf_left->planes, 0);

  if (is_float) {
    imb_addrectfloatImBuf(ibuf_stereo, ibuf_left->channels);
    ibuf_stereo->float_buffer.colorspace = ibuf_left->float_buffer.colorspace;
  }
  else {
    imb_addrectImBuf(ibuf_stereo);
    ibuf_stereo->byte_buffer.colorspace = ibuf_left->byte_buffer.colorspace;
  }

  ibuf_stereo->flags = ibuf_left->flags;

  imb_stereo3d_data_init(&s3d_data,
                         is_float,
                         ibuf_left->x,
                         ibuf_left->y,
                         ibuf_left->channels,
                         (int *)ibuf_left->byte_buffer.data,
                         (int *)ibuf_right->byte_buffer.data,
                         (int *)ibuf_stereo->byte_buffer.data,
                         ibuf_left->float_buffer.data,
                         ibuf_right->float_buffer.data,
                         ibuf_stereo->float_buffer.data);

  imb_stereo3d_write_doit(&s3d_data, &im_format->stereo3d_format);
  imb_stereo3d_squeeze_ImBuf(ibuf_stereo, &im_format->stereo3d_format, ibuf_left->x, ibuf_left->y);

  return ibuf_stereo;
}

static void imb_stereo3d_write_doit(Stereo3DData *s3d_data, const Stereo3dFormat *s3d)
{
  switch (s3d->display_mode) {
    case S3D_DISPLAY_ANAGLYPH:
      imb_stereo3d_write_anaglyph(s3d_data, eStereo3dAnaglyphType(s3d->anaglyph_type));
      break;
    case S3D_DISPLAY_INTERLACE:
      imb_stereo3d_write_interlace(s3d_data,
                                   eStereo3dInterlaceType(s3d->interlace_type),
                                   (s3d->flag & S3D_INTERLACE_SWAP) != 0);
      break;
    case S3D_DISPLAY_SIDEBYSIDE:
      imb_stereo3d_write_sidebyside(s3d_data, (s3d->flag & S3D_SIDEBYSIDE_CROSSEYED) != 0);
      break;
    case S3D_DISPLAY_TOPBOTTOM:
      imb_stereo3d_write_topbottom(s3d_data);
      break;
    default:
      break;
  }
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Reading Stereo ImBuf's
 * \{ */

static void imb_stereo3d_read_anaglyph(const Stereo3DData *s3d, enum eStereo3dAnaglyphType mode)
{
  int x, y;
  size_t width = s3d->x;
  size_t height = s3d->y;
  const size_t channels = s3d->channels;

  const int stride_from = width;
  const int stride_to = width;

  const int anaglyph_encoding[3][3] = {
      {0, 1, 1},
      {1, 0, 1},
      {0, 0, 1},
  };

  int r, g, b;

  r = anaglyph_encoding[mode][0];
  g = anaglyph_encoding[mode][1];
  b = anaglyph_encoding[mode][2];

  if (s3d->is_float) {
    float *rect_left = s3d->rectf.left;
    float *rect_right = s3d->rectf.right;
    float *rect_from = s3d->rectf.stereo;

    if (channels == 3) {
      for (y = 0; y < height; y++) {
        float *from = rect_from + stride_from * y * 3;
        float *to[2] = {
            rect_left + stride_to * y * 3,
            rect_right + stride_to * y * 3,
        };

        for (x = 0; x < width; x++, from += 3, to[0] += 3, to[1] += 3) {
          to[r][0] = from[0];
          to[g][1] = from[1];
          to[b][2] = from[2];
        }
      }
    }
    else if (channels == 4) {
      for (y = 0; y < height; y++) {
        float *from = rect_from + stride_from * y * 4;
        float *to[2] = {
            rect_left + stride_to * y * 4,
            rect_right + stride_to * y * 4,
        };

        for (x = 0; x < width; x++, from += 4, to[0] += 4, to[1] += 4) {
          to[r][0] = from[0];
          to[g][1] = from[1];
          to[b][2] = from[2];
          to[0][3] = to[1][3] = from[3];
        }
      }
    }
  }
  else {
    uchar *rect_left = s3d->rect.left;
    uchar *rect_right = s3d->rect.right;
    uchar *rect_from = s3d->rect.stereo;

    if (channels == 3) {
      for (y = 0; y < height; y++) {
        uchar *from = rect_from + stride_from * y * 3;
        uchar *to[2] = {
            rect_left + stride_to * y * 3,
            rect_right + stride_to * y * 3,
        };

        for (x = 0; x < width; x++, from += 3, to[0] += 3, to[1] += 3) {
          to[r][0] = from[0];
          to[g][1] = from[1];
          to[b][2] = from[2];
        }
      }
    }
    else if (channels == 4) {
      for (y = 0; y < height; y++) {
        uchar *from = rect_from + stride_from * y * 4;
        uchar *to[2] = {
            rect_left + stride_to * y * 4,
            rect_right + stride_to * y * 4,
        };

        for (x = 0; x < width; x++, from += 4, to[0] += 4, to[1] += 4) {
          to[r][0] = from[0];
          to[g][1] = from[1];
          to[b][2] = from[2];
          to[0][3] = to[1][3] = from[3];
        }
      }
    }
  }
}

static void imb_stereo3d_read_interlace(const Stereo3DData *s3d,
                                        enum eStereo3dInterlaceType mode,
                                        const bool swap)
{
  int x, y;
  size_t width = s3d->x;
  size_t height = s3d->y;
  const size_t channels = s3d->channels;

  const int stride_from = width;
  const int stride_to = width;

  if (s3d->is_float) {
    float *rect_left = s3d->rectf.left;
    float *rect_right = s3d->rectf.right;
    const float *rect_from = s3d->rectf.stereo;

    switch (mode) {
      case S3D_INTERLACE_ROW: {
        char i = char(swap);
        for (y = 0; y < height; y++) {
          const float *from = rect_from + stride_from * y * channels;
          float *to[2] = {
              rect_left + stride_to * y * channels,
              rect_right + stride_to * y * channels,
          };
          memcpy(to[i], from, sizeof(float) * channels * stride_to);
          i = !i;
        }
        break;
      }
      case S3D_INTERLACE_COLUMN: {
        if (channels == 1) {
          for (y = 0; y < height; y++) {
            const float *from = rect_from + stride_from * y;
            float *to[2] = {
                rect_left + stride_to * y,
                rect_right + stride_to * y,
            };

            char i = char(swap);
            for (x = 0; x < width; x++, from += 1, to[0] += 1, to[1] += 1) {
              to[i][0] = from[0];
              i = !i;
            }
          }
        }
        else if (channels == 3) {
          for (y = 0; y < height; y++) {
            const float *from = rect_from + stride_from * y * 3;
            float *to[2] = {
                rect_left + stride_to * y * 3,
                rect_right + stride_to * y * 3,
            };

            char i = char(swap);
            for (x = 0; x < width; x++, from += 3, to[0] += 3, to[1] += 3) {
              copy_v3_v3(to[i], from);
              i = !i;
            }
          }
        }
        else if (channels == 4) {
          for (y = 0; y < height; y++) {
            const float *from = rect_from + stride_from * y * channels;
            float *to[2] = {
                rect_left + stride_to * y * channels,
                rect_right + stride_to * y * channels,
            };

            char i = char(swap);
            for (x = 0; x < width; x++, from += 4, to[0] += 4, to[1] += 4) {
              copy_v4_v4(to[i], from);
              i = !i;
            }
          }
        }
        break;
      }
      case S3D_INTERLACE_CHECKERBOARD: {
        if (channels == 1) {
          char i = char(swap);
          for (y = 0; y < height; y++) {
            const float *from = rect_from + stride_from * y;
            float *to[2] = {
                rect_left + stride_to * y,
                rect_right + stride_to * y,
            };
            char j = i;
            for (x = 0; x < width; x++, from += 1, to[0] += 1, to[1] += 1) {
              to[j][0] = from[0];
              j = !j;
            }
            i = !i;
          }
        }
        else if (channels == 3) {
          char i = char(swap);
          for (y = 0; y < height; y++) {
            const float *from = rect_from + stride_from * y * 3;
            float *to[2] = {
                rect_left + stride_to * y * 3,
                rect_right + stride_to * y * 3,
            };
            char j = i;
            for (x = 0; x < width; x++, from += 3, to[0] += 3, to[1] += 3) {
              copy_v3_v3(to[j], from);
              j = !j;
            }
            i = !i;
          }
        }
        else if (channels == 4) {
          char i = char(swap);
          for (y = 0; y < height; y++) {
            const float *from = rect_from + stride_from * y * 4;
            float *to[2] = {
                rect_left + stride_to * y * 4,
                rect_right + stride_to * y * 4,
            };
            char j = i;
            for (x = 0; x < width; x++, from += 4, to[0] += 4, to[1] += 4) {
              copy_v4_v4(to[j], from);
              j = !j;
            }
            i = !i;
          }
        }
        break;
      }
      default: {
        break;
      }
    }
  }
  else {
    uchar *rect_left = s3d->rect.right;
    uchar *rect_right = s3d->rect.left;
    const uchar *rect_from = s3d->rect.stereo;

    switch (mode) {
      case S3D_INTERLACE_ROW: {
        char i = char(swap);
        for (y = 0; y < height; y++) {
          const uchar *from = rect_from + stride_from * y * channels;
          uchar *to[2] = {
              rect_left + stride_to * y * channels,
              rect_right + stride_to * y * channels,
          };
          memcpy(to[i], from, sizeof(uchar) * channels * stride_to);
          i = !i;
        }
        break;
      }
      case S3D_INTERLACE_COLUMN: {
        if (channels == 1) {
          for (y = 0; y < height; y++) {
            const uchar *from = rect_from + stride_from * y;
            uchar *to[2] = {
                rect_left + stride_to * y,
                rect_right + stride_to * y,
            };
            char i = char(swap);
            for (x = 0; x < width; x++, from += 1, to[0] += 1, to[1] += 1) {
              to[i][0] = from[0];
              i = !i;
            }
          }
        }
        else if (channels == 3) {
          for (y = 0; y < height; y++) {
            const uchar *from = rect_from + stride_from * y * 3;
            uchar *to[2] = {
                rect_left + stride_to * y * 3,
                rect_right + stride_to * y * 3,
            };
            char i = char(swap);
            for (x = 0; x < width; x++, from += 3, to[0] += 3, to[1] += 3) {
              copy_v3_v3_uchar(to[i], from);
              i = !i;
            }
          }
        }
        else if (channels == 4) {
          for (y = 0; y < height; y++) {
            const uchar *from = rect_from + stride_from * y * 4;
            uchar *to[2] = {
                rect_left + stride_to * y * 4,
                rect_right + stride_to * y * 4,
            };
            char i = char(swap);
            for (x = 0; x < width; x++, from += 4, to[0] += 4, to[1] += 4) {
              copy_v4_v4_uchar(to[i], from);
              i = !i;
            }
          }
        }
        break;
      }
      case S3D_INTERLACE_CHECKERBOARD: {
        if (channels == 1) {
          char i = char(swap);
          for (y = 0; y < height; y++) {
            const uchar *from = rect_from + stride_from * y;
            uchar *to[2] = {
                rect_left + stride_to * y,
                rect_right + stride_to * y,
            };
            char j = i;
            for (x = 0; x < width; x++, from += 1, to[0] += 1, to[1] += 1) {
              to[j][0] = from[0];
              j = !j;
            }
            i = !i;
          }
        }
        else if (channels == 3) {
          char i = char(swap);
          for (y = 0; y < height; y++) {
            const uchar *from = rect_from + stride_from * y * 3;
            uchar *to[2] = {
                rect_left + stride_to * y * 3,
                rect_right + stride_to * y * 3,
            };
            char j = i;
            for (x = 0; x < width; x++, from += 3, to[0] += 3, to[1] += 3) {
              copy_v3_v3_uchar(to[j], from);
              j = !j;
            }
            i = !i;
          }
        }
        else if (channels == 4) {
          char i = char(swap);
          for (y = 0; y < height; y++) {
            const uchar *from = rect_from + stride_from * y * 4;
            uchar *to[2] = {
                rect_left + stride_to * y * 4,
                rect_right + stride_to * y * 4,
            };
            char j = i;
            for (x = 0; x < width; x++, from += 4, to[0] += 4, to[1] += 4) {
              copy_v4_v4_uchar(to[j], from);
              j = !j;
            }
            i = !i;
          }
        }
        break;
      }
      default: {
        break;
      }
    }
  }
}

/* stereo input (s3d->rectf.stereo) is always unsqueezed */
static void imb_stereo3d_read_sidebyside(const Stereo3DData *s3d, const bool crosseyed)
{
  int y;
  size_t width = s3d->x;
  size_t height = s3d->y;
  const size_t channels = s3d->channels;

  const int stride_from = width * 2;
  const int stride_to = width;

  const int l = int(crosseyed);
  const int r = !l;

  if (s3d->is_float) {
    float *rect_left = s3d->rectf.left;
    float *rect_right = s3d->rectf.right;
    const float *rect_from = s3d->rectf.stereo;

    for (y = 0; y < height; y++) {
      const float *from = rect_from + stride_from * y * channels;
      float *to[2] = {
          rect_left + stride_to * y * channels,
          rect_right + stride_to * y * channels,
      };

      memcpy(to[l], from, sizeof(float) * channels * stride_to);
      memcpy(to[r], from + channels * stride_to, sizeof(float) * channels * stride_to);
    }
  }
  else {
    uchar *rect_left = s3d->rect.left;
    uchar *rect_right = s3d->rect.right;
    const uchar *rect_from = s3d->rect.stereo;

    /* always RGBA input/output */
    for (y = 0; y < height; y++) {
      const uchar *from = rect_from + stride_from * y * channels;
      uchar *to[2] = {
          rect_left + stride_to * y * channels,
          rect_right + stride_to * y * channels,
      };

      memcpy(to[l], from, sizeof(uchar) * channels * stride_to);
      memcpy(to[r], from + channels * stride_to, sizeof(uchar) * channels * stride_to);
    }
  }
}

/* stereo input (s3d->rectf.stereo) is always unsqueezed */
static void imb_stereo3d_read_topbottom(const Stereo3DData *s3d)
{
  int y;
  size_t width = s3d->x;
  size_t height = s3d->y;
  const size_t channels = s3d->channels;

  const int stride_from = width;
  const int stride_to = width;

  if (s3d->is_float) {
    float *rect_left = s3d->rectf.left;
    float *rect_right = s3d->rectf.right;
    const float *rect_from = s3d->rectf.stereo;

    for (y = 0; y < height; y++) {
      const float *from = rect_from + stride_from * y * channels;
      float *to[2] = {
          rect_left + stride_to * y * channels,
          rect_right + stride_to * y * channels,
      };

      memcpy(to[1], from, sizeof(float) * channels * stride_to);
      memcpy(to[0], from + channels * height * stride_to, sizeof(float) * channels * stride_to);
    }
  }
  else {
    uchar *rect_left = s3d->rect.left;
    uchar *rect_right = s3d->rect.right;
    const uchar *rect_from = s3d->rect.stereo;

    for (y = 0; y < height; y++) {
      const uchar *from = rect_from + stride_from * y * channels;
      uchar *to[2] = {
          rect_left + stride_to * y * channels,
          rect_right + stride_to * y * channels,
      };

      memcpy(to[1], from, sizeof(uchar) * channels * stride_to);
      memcpy(to[0], from + channels * height * stride_to, sizeof(uchar) * channels * stride_to);
    }
  }
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Preparing To Call The Read Functions
 * \{ */

void IMB_ImBufFromStereo3d(const Stereo3dFormat *s3d,
                           ImBuf *ibuf_stereo3d,
                           ImBuf **r_ibuf_left,
                           ImBuf **r_ibuf_right)
{
  Stereo3DData s3d_data = {{nullptr}};
  ImBuf *ibuf_left, *ibuf_right;
  size_t width, height;
  const bool is_float = (ibuf_stereo3d->float_buffer.data != nullptr);

  IMB_stereo3d_read_dimensions(s3d->display_mode,
                               ((s3d->flag & S3D_SQUEEZED_FRAME) == 0),
                               ibuf_stereo3d->x,
                               ibuf_stereo3d->y,
                               &width,
                               &height);

  ibuf_left = IMB_allocImBuf(width, height, ibuf_stereo3d->planes, 0);
  ibuf_right = IMB_allocImBuf(width, height, ibuf_stereo3d->planes, 0);

  if (is_float) {
    imb_addrectfloatImBuf(ibuf_left, ibuf_stereo3d->channels);
    imb_addrectfloatImBuf(ibuf_right, ibuf_stereo3d->channels);
  }
  else {
    imb_addrectImBuf(ibuf_left);
    imb_addrectImBuf(ibuf_right);
  }

  ibuf_left->flags = ibuf_stereo3d->flags;
  ibuf_right->flags = ibuf_stereo3d->flags;

  /* we always work with unsqueezed formats */
  IMB_stereo3d_write_dimensions(s3d->display_mode,
                                ((s3d->flag & S3D_SQUEEZED_FRAME) == 0),
                                ibuf_stereo3d->x,
                                ibuf_stereo3d->y,
                                &width,
                                &height);
  imb_stereo3d_unsqueeze_ImBuf(ibuf_stereo3d, s3d, width, height);

  imb_stereo3d_data_init(&s3d_data,
                         is_float,
                         ibuf_left->x,
                         ibuf_left->y,
                         ibuf_left->channels,
                         (int *)ibuf_left->byte_buffer.data,
                         (int *)ibuf_right->byte_buffer.data,
                         (int *)ibuf_stereo3d->byte_buffer.data,
                         ibuf_left->float_buffer.data,
                         ibuf_right->float_buffer.data,
                         ibuf_stereo3d->float_buffer.data);

  imb_stereo3d_read_doit(&s3d_data, s3d);

  IMB_freeImBuf(ibuf_stereo3d);

  *r_ibuf_left = ibuf_left;
  *r_ibuf_right = ibuf_right;
}

static void imb_stereo3d_read_doit(Stereo3DData *s3d_data, const Stereo3dFormat *s3d)
{
  switch (s3d->display_mode) {
    case S3D_DISPLAY_ANAGLYPH:
      imb_stereo3d_read_anaglyph(s3d_data, eStereo3dAnaglyphType(s3d->anaglyph_type));
      break;
    case S3D_DISPLAY_INTERLACE:
      imb_stereo3d_read_interlace(s3d_data,
                                  eStereo3dInterlaceType(s3d->interlace_type),
                                  (s3d->flag & S3D_INTERLACE_SWAP) != 0);
      break;
    case S3D_DISPLAY_SIDEBYSIDE:
      imb_stereo3d_read_sidebyside(s3d_data, (s3d->flag & S3D_SIDEBYSIDE_CROSSEYED) != 0);
      break;
    case S3D_DISPLAY_TOPBOTTOM:
      imb_stereo3d_read_topbottom(s3d_data);
      break;
    default:
      break;
  }
}

/** \} */
