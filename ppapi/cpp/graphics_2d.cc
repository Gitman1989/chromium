// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/graphics_2d.h"

#include "ppapi/c/pp_errors.h"
#include "ppapi/c/ppb_graphics_2d.h"
#include "ppapi/cpp/common.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/image_data.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/module_impl.h"
#include "ppapi/cpp/point.h"
#include "ppapi/cpp/rect.h"

namespace pp {

namespace {

template <> const char* interface_name<PPB_Graphics2D>() {
  return PPB_GRAPHICS_2D_INTERFACE;
}

}  // namespace

Graphics2D::Graphics2D() : Resource() {
}

Graphics2D::Graphics2D(const Graphics2D& other)
    : Resource(other),
      size_(other.size_) {
}

Graphics2D::Graphics2D(const Size& size, bool is_always_opaque)
    : Resource() {
  if (!has_interface<PPB_Graphics2D>())
    return;
  PassRefFromConstructor(get_interface<PPB_Graphics2D>()->Create(
      Module::Get()->pp_module(),
      &size.pp_size(),
      BoolToPPBool(is_always_opaque)));
  if (!is_null()) {
    // Only save the size if allocation succeeded.
    size_ = size;
  }
}

Graphics2D::~Graphics2D() {
}

Graphics2D& Graphics2D::operator=(const Graphics2D& other) {
  Graphics2D copy(other);
  swap(copy);
  return *this;
}

void Graphics2D::swap(Graphics2D& other) {
  Resource::swap(other);
  size_.swap(other.size_);
}

void Graphics2D::PaintImageData(const ImageData& image,
                                const Point& top_left) {
  if (!has_interface<PPB_Graphics2D>())
    return;
  get_interface<PPB_Graphics2D>()->PaintImageData(pp_resource(),
                                                  image.pp_resource(),
                                                  &top_left.pp_point(),
                                                  NULL);
}

void Graphics2D::PaintImageData(const ImageData& image,
                                const Point& top_left,
                                const Rect& src_rect) {
  if (!has_interface<PPB_Graphics2D>())
    return;
  get_interface<PPB_Graphics2D>()->PaintImageData(pp_resource(),
                                                  image.pp_resource(),
                                                  &top_left.pp_point(),
                                                  &src_rect.pp_rect());
}

void Graphics2D::Scroll(const Rect& clip, const Point& amount) {
  if (!has_interface<PPB_Graphics2D>())
    return;
  get_interface<PPB_Graphics2D>()->Scroll(pp_resource(),
                                          &clip.pp_rect(),
                                          &amount.pp_point());
}

void Graphics2D::ReplaceContents(ImageData* image) {
  if (!has_interface<PPB_Graphics2D>())
    return;
  get_interface<PPB_Graphics2D>()->ReplaceContents(pp_resource(),
                                                   image->pp_resource());

  // On success, reset the image data. This is to help prevent people
  // from continuing to use the resource which will result in artifacts.
  *image = ImageData();
}

int32_t Graphics2D::Flush(const CompletionCallback& cc) {
  if (!has_interface<PPB_Graphics2D>())
    return PP_ERROR_NOINTERFACE;
  return get_interface<PPB_Graphics2D>()->Flush(pp_resource(),
                                                cc.pp_completion_callback());
}

}  // namespace pp
