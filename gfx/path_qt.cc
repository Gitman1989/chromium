// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gfx/path.h"

#include "base/scoped_ptr.h"
#include "base/command_line.h"

namespace gfx {

NativeRegion Path::CreateNativeRegion() const {
   return NULL;
}

// static
NativeRegion Path::IntersectRegions(NativeRegion r1, NativeRegion r2) {
  return NULL;
}

// static
NativeRegion Path::CombineRegions(NativeRegion r1, NativeRegion r2) {
  return NULL;
}

// static
NativeRegion Path::SubtractRegion(NativeRegion r1, NativeRegion r2) {
  return NULL;
}

}  // namespace gfx
