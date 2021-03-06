// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#define _USE_MATH_DEFINES
#include <cmath>
#include <limits>

#include "skia/ext/image_operations.h"

// TODO(pkasting): skia/ext should not depend on base/!
#include "base/logging.h"
#include "base/metrics/histogram.h"
#include "base/stack_container.h"
#include "base/time.h"
#include "build/build_config.h"
#include "skia/ext/convolver.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkRect.h"
#include "third_party/skia/include/core/SkFontHost.h"
#include "third_party/skia/include/core/SkColorPriv.h"

namespace skia {

namespace {

// Returns the ceiling/floor as an integer.
inline int CeilInt(float val) {
  return static_cast<int>(ceil(val));
}
inline int FloorInt(float val) {
  return static_cast<int>(floor(val));
}

// Filter function computation -------------------------------------------------

// Evaluates the box filter, which goes from -0.5 to +0.5.
float EvalBox(float x) {
  return (x >= -0.5f && x < 0.5f) ? 1.0f : 0.0f;
}

// Evaluates the Lanczos filter of the given filter size window for the given
// position.
//
// |filter_size| is the width of the filter (the "window"), outside of which
// the value of the function is 0. Inside of the window, the value is the
// normalized sinc function:
//   lanczos(x) = sinc(x) * sinc(x / filter_size);
// where
//   sinc(x) = sin(pi*x) / (pi*x);
float EvalLanczos(int filter_size, float x) {
  if (x <= -filter_size || x >= filter_size)
    return 0.0f;  // Outside of the window.
  if (x > -std::numeric_limits<float>::epsilon() &&
      x < std::numeric_limits<float>::epsilon())
    return 1.0f;  // Special case the discontinuity at the origin.
  float xpi = x * static_cast<float>(M_PI);
  return (sin(xpi) / xpi) *  // sinc(x)
          sin(xpi / filter_size) / (xpi / filter_size);  // sinc(x/filter_size)
}

// ResizeFilter ----------------------------------------------------------------

// Encapsulates computation and storage of the filters required for one complete
// resize operation.
class ResizeFilter {
 public:
  ResizeFilter(ImageOperations::ResizeMethod method,
               int src_full_width, int src_full_height,
               int dest_width, int dest_height,
               const SkIRect& dest_subset);

  // Returns the bounds in the input bitmap of data that is used in the output.
  // The filter offsets are within this rectangle.
  const SkIRect& src_depend() { return src_depend_; }

  // Returns the filled filter values.
  const ConvolutionFilter1D& x_filter() { return x_filter_; }
  const ConvolutionFilter1D& y_filter() { return y_filter_; }

 private:
  // Returns the number of pixels that the filer spans, in filter space (the
  // destination image).
  float GetFilterSupport(float scale) {
    switch (method_) {
      case ImageOperations::RESIZE_BOX:
        // The box filter just scales with the image scaling.
        return 0.5f;  // Only want one side of the filter = /2.
      case ImageOperations::RESIZE_LANCZOS3:
        // The lanczos filter takes as much space in the source image in
        // each direction as the size of the window = 3 for Lanczos3.
        return 3.0f;
      default:
        NOTREACHED();
        return 1.0f;
    }
  }

  // Computes one set of filters either horizontally or vertically. The caller
  // will specify the "min" and "max" rather than the bottom/top and
  // right/bottom so that the same code can be re-used in each dimension.
  //
  // |src_depend_lo| and |src_depend_size| gives the range for the source
  // depend rectangle (horizontally or vertically at the caller's discretion
  // -- see above for what this means).
  //
  // Likewise, the range of destination values to compute and the scale factor
  // for the transform is also specified.
  void ComputeFilters(int src_size,
                      int dest_subset_lo, int dest_subset_size,
                      float scale, float src_support,
                      ConvolutionFilter1D* output);

  // Computes the filter value given the coordinate in filter space.
  inline float ComputeFilter(float pos) {
    switch (method_) {
      case ImageOperations::RESIZE_BOX:
        return EvalBox(pos);
      case ImageOperations::RESIZE_LANCZOS3:
        return EvalLanczos(3, pos);
      default:
        NOTREACHED();
        return 0;
    }
  }

  ImageOperations::ResizeMethod method_;

  // Subset of source the filters will touch.
  SkIRect src_depend_;

  // Size of the filter support on one side only in the destination space.
  // See GetFilterSupport.
  float x_filter_support_;
  float y_filter_support_;

  // Subset of scaled destination bitmap to compute.
  SkIRect out_bounds_;

  ConvolutionFilter1D x_filter_;
  ConvolutionFilter1D y_filter_;

  DISALLOW_COPY_AND_ASSIGN(ResizeFilter);
};

ResizeFilter::ResizeFilter(ImageOperations::ResizeMethod method,
                           int src_full_width, int src_full_height,
                           int dest_width, int dest_height,
                           const SkIRect& dest_subset)
    : method_(method),
      out_bounds_(dest_subset) {
  float scale_x = static_cast<float>(dest_width) /
                  static_cast<float>(src_full_width);
  float scale_y = static_cast<float>(dest_height) /
                  static_cast<float>(src_full_height);

  x_filter_support_ = GetFilterSupport(scale_x);
  y_filter_support_ = GetFilterSupport(scale_y);

  SkIRect src_full = { 0, 0, src_full_width, src_full_height };
  SkIRect dest_full = { 0, 0, static_cast<int>(src_full_width * scale_x + 0.5),
                        static_cast<int>(src_full_height * scale_y + 0.5) };

  // Support of the filter in source space.
  float src_x_support = x_filter_support_ / scale_x;
  float src_y_support = y_filter_support_ / scale_y;

  ComputeFilters(src_full_width, dest_subset.fLeft, dest_subset.width(),
                 scale_x, src_x_support, &x_filter_);
  ComputeFilters(src_full_height, dest_subset.fTop, dest_subset.height(),
                 scale_y, src_y_support, &y_filter_);
}

void ResizeFilter::ComputeFilters(int src_size,
                                  int dest_subset_lo, int dest_subset_size,
                                  float scale, float src_support,
                                  ConvolutionFilter1D* output) {
  int dest_subset_hi = dest_subset_lo + dest_subset_size;  // [lo, hi)

  // When we're doing a magnification, the scale will be larger than one. This
  // means the destination pixels are much smaller than the source pixels, and
  // that the range covered by the filter won't necessarily cover any source
  // pixel boundaries. Therefore, we use these clamped values (max of 1) for
  // some computations.
  float clamped_scale = std::min(1.0f, scale);

  // Speed up the divisions below by turning them into multiplies.
  float inv_scale = 1.0f / scale;

  StackVector<float, 64> filter_values;
  StackVector<int16, 64> fixed_filter_values;

  // Loop over all pixels in the output range. We will generate one set of
  // filter values for each one. Those values will tell us how to blend the
  // source pixels to compute the destination pixel.
  for (int dest_subset_i = dest_subset_lo; dest_subset_i < dest_subset_hi;
       dest_subset_i++) {
    // Reset the arrays. We don't declare them inside so they can re-use the
    // same malloc-ed buffer.
    filter_values->clear();
    fixed_filter_values->clear();

    // This is the pixel in the source directly under the pixel in the dest.
    float src_pixel = dest_subset_i * inv_scale;

    // Compute the (inclusive) range of source pixels the filter covers.
    int src_begin = std::max(0, FloorInt(src_pixel - src_support));
    int src_end = std::min(src_size - 1, CeilInt(src_pixel + src_support));

    // Compute the unnormalized filter value at each location of the source
    // it covers.
    float filter_sum = 0.0f;  // Sub of the filter values for normalizing.
    for (int cur_filter_pixel = src_begin; cur_filter_pixel <= src_end;
         cur_filter_pixel++) {
      // Distance from the center of the filter, this is the filter coordinate
      // in source space.
      float src_filter_pos = cur_filter_pixel - src_pixel;

      // Since the filter really exists in dest space, map it there.
      float dest_filter_pos = src_filter_pos * clamped_scale;

      // Compute the filter value at that location.
      float filter_value = ComputeFilter(dest_filter_pos);
      filter_values->push_back(filter_value);

      filter_sum += filter_value;
    }
    DCHECK(!filter_values->empty()) << "We should always get a filter!";

    // The filter must be normalized so that we don't affect the brightness of
    // the image. Convert to normalized fixed point.
    int16 fixed_sum = 0;
    for (size_t i = 0; i < filter_values->size(); i++) {
      int16 cur_fixed = output->FloatToFixed(filter_values[i] / filter_sum);
      fixed_sum += cur_fixed;
      fixed_filter_values->push_back(cur_fixed);
    }

    // The conversion to fixed point will leave some rounding errors, which
    // we add back in to avoid affecting the brightness of the image. We
    // arbitrarily add this to the center of the filter array (this won't always
    // be the center of the filter function since it could get clipped on the
    // edges, but it doesn't matter enough to worry about that case).
    int16 leftovers = output->FloatToFixed(1.0f) - fixed_sum;
    fixed_filter_values[fixed_filter_values->size() / 2] += leftovers;

    // Now it's ready to go.
    output->AddFilter(src_begin, &fixed_filter_values[0],
                      static_cast<int>(fixed_filter_values->size()));
  }
}

}  // namespace

// Resize ----------------------------------------------------------------------

// static
SkBitmap ImageOperations::Resize(const SkBitmap& source,
                                 ResizeMethod method,
                                 int dest_width, int dest_height,
                                 const SkIRect& dest_subset) {
  if (method == ImageOperations::RESIZE_SUBPIXEL)
    return ResizeSubpixel(source, dest_width, dest_height, dest_subset);
  else
    return ResizeBasic(source, method, dest_width, dest_height, dest_subset);
}

// static
SkBitmap ImageOperations::ResizeSubpixel(const SkBitmap& source,
                                         int dest_width, int dest_height,
                                         const SkIRect& dest_subset) {
  // Currently only works on Linux/BSD because these are the only platforms
  // where SkFontHost::GetSubpixelOrder is defined.
#if defined(OS_POSIX) && !defined(OS_MACOSX)
  // Understand the display.
  const SkFontHost::LCDOrder order = SkFontHost::GetSubpixelOrder();
  const SkFontHost::LCDOrientation orientation =
      SkFontHost::GetSubpixelOrientation();

  // Decide on which dimension, if any, to deploy subpixel rendering.
  int w = 1;
  int h = 1;
  switch (orientation) {
    case SkFontHost::kHorizontal_LCDOrientation:
      w = dest_width < source.width() ? 3 : 1;
      break;
    case SkFontHost::kVertical_LCDOrientation:
      h = dest_height < source.height() ? 3 : 1;
      break;
  }

  // Resize the image.
  const int width = dest_width * w;
  const int height = dest_height * h;
  SkIRect subset = { dest_subset.fLeft, dest_subset.fTop,
                     dest_subset.fLeft + dest_subset.width() * w,
                     dest_subset.fTop + dest_subset.height() * h };
  SkBitmap img = ResizeBasic(source, ImageOperations::RESIZE_LANCZOS3, width,
                             height, subset);
  const int row_words = img.rowBytes() / 4;
  if (w == 1 && h == 1)
    return img;

  // Render into subpixels.
  SkBitmap result;
  result.setConfig(SkBitmap::kARGB_8888_Config, dest_subset.width(),
                   dest_subset.height());
  result.allocPixels();
  SkAutoLockPixels locker(img);
  uint32* src_row = img.getAddr32(0, 0);
  uint32* dst_row = result.getAddr32(0, 0);
  for (int y = 0; y < dest_subset.height(); y++) {
    uint32* src = src_row;
    uint32* dst = dst_row;
    for (int x = 0; x < dest_subset.width(); x++, src += w, dst++) {
      uint8 r, g, b, a;
      switch (order) {
        case SkFontHost::kRGB_LCDOrder:
          switch (orientation) {
            case SkFontHost::kHorizontal_LCDOrientation:
              r = SkGetPackedR32(src[0]);
              g = SkGetPackedG32(src[1]);
              b = SkGetPackedB32(src[2]);
              a = SkGetPackedA32(src[1]);
              break;
            case SkFontHost::kVertical_LCDOrientation:
              r = SkGetPackedR32(src[0 * row_words]);
              g = SkGetPackedG32(src[1 * row_words]);
              b = SkGetPackedB32(src[2 * row_words]);
              a = SkGetPackedA32(src[1 * row_words]);
              break;
          }
          break;
        case SkFontHost::kBGR_LCDOrder:
          switch (orientation) {
            case SkFontHost::kHorizontal_LCDOrientation:
              b = SkGetPackedB32(src[0]);
              g = SkGetPackedG32(src[1]);
              r = SkGetPackedR32(src[2]);
              a = SkGetPackedA32(src[1]);
              break;
            case SkFontHost::kVertical_LCDOrientation:
              b = SkGetPackedB32(src[0 * row_words]);
              g = SkGetPackedG32(src[1 * row_words]);
              r = SkGetPackedR32(src[2 * row_words]);
              a = SkGetPackedA32(src[1 * row_words]);
              break;
          }
          break;
      }
      // Premultiplied alpha is very fragile.
      a = a > r ? a : r;
      a = a > g ? a : g;
      a = a > b ? a : b;
      *dst = SkPackARGB32(a, r, g, b);
    }
    src_row += h * row_words;
    dst_row += result.rowBytes() / 4;
  }
  result.setIsOpaque(img.isOpaque());
  return result;
#else
  return SkBitmap();
#endif  // OS_POSIX && !OS_MACOSX
}

// static
SkBitmap ImageOperations::ResizeBasic(const SkBitmap& source,
                                      ResizeMethod method,
                                      int dest_width, int dest_height,
                                      const SkIRect& dest_subset) {
  // Time how long this takes to see if it's a problem for users.
  base::TimeTicks resize_start = base::TimeTicks::Now();

  SkIRect dest = { 0, 0, dest_width, dest_height };
  DCHECK(dest.contains(dest_subset)) <<
      "The supplied subset does not fall within the destination image.";

  // If the size of source or destination is 0, i.e. 0x0, 0xN or Nx0, just
  // return empty.
  if (source.width() < 1 || source.height() < 1 ||
      dest_width < 1 || dest_height < 1)
    return SkBitmap();

  SkAutoLockPixels locker(source);

  ResizeFilter filter(method, source.width(), source.height(),
                      dest_width, dest_height, dest_subset);

  // Get a source bitmap encompassing this touched area. We construct the
  // offsets and row strides such that it looks like a new bitmap, while
  // referring to the old data.
  const uint8* source_subset =
      reinterpret_cast<const uint8*>(source.getPixels());

  // Convolve into the result.
  SkBitmap result;
  result.setConfig(SkBitmap::kARGB_8888_Config,
                   dest_subset.width(), dest_subset.height());
  result.allocPixels();
  BGRAConvolve2D(source_subset, static_cast<int>(source.rowBytes()),
                 !source.isOpaque(), filter.x_filter(), filter.y_filter(),
                 static_cast<unsigned char*>(result.getPixels()));

  // Preserve the "opaque" flag for use as an optimization later.
  result.setIsOpaque(source.isOpaque());

  base::TimeDelta delta = base::TimeTicks::Now() - resize_start;
  UMA_HISTOGRAM_TIMES("Image.ResampleMS", delta);

  return result;
}

// static
SkBitmap ImageOperations::Resize(const SkBitmap& source,
                                 ResizeMethod method,
                                 int dest_width, int dest_height) {
  SkIRect dest_subset = { 0, 0, dest_width, dest_height };
  return Resize(source, method, dest_width, dest_height, dest_subset);
}

}  // namespace skia
