// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gfx/platform_font_qt.h"

namespace gfx {

////////////////////////////////////////////////////////////////////////////////
// PlatformFont, public:

// static
PlatformFont* PlatformFont::CreateDefault() {
  return NULL;
}

// static
PlatformFont* PlatformFont::CreateFromFont(const Font& other) {
  return NULL;
}

// static
PlatformFont* PlatformFont::CreateFromNativeFont(NativeFont native_font) {
  return NULL;
}

// static
PlatformFont* PlatformFont::CreateFromNameAndSize(const string16& font_name,
                                                  int font_size) {
  return NULL;
}

}  // namespace gfx
