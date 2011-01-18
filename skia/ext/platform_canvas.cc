// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "skia/ext/platform_canvas.h"

#include "skia/ext/bitmap_platform_device.h"
#include "third_party/skia/include/core/SkTypes.h"

namespace skia {

PlatformCanvas::PlatformCanvas()
    : SkCanvas(SkNEW(BitmapPlatformDeviceFactory)) {
}

PlatformCanvas::PlatformCanvas(SkDeviceFactory* factory) : SkCanvas(factory) {
}

SkDevice* PlatformCanvas::setBitmapDevice(const SkBitmap&) {
  SkASSERT(false);  // Should not be called.
  return NULL;
}

PlatformDevice& PlatformCanvas::getTopPlatformDevice() const {
  // All of our devices should be our special PlatformDevice.
  SkCanvas::LayerIter iter(const_cast<PlatformCanvas*>(this), false);
  return *static_cast<PlatformDevice*>(iter.device());
}

// static
size_t PlatformCanvas::StrideForWidth(unsigned width) {
  return 4 * width;
}

bool PlatformCanvas::initializeWithDevice(SkDevice* device) {
  if (!device)
    return false;

  setDevice(device);
  device->unref();  // Was created with refcount 1, and setDevice also refs.
  return true;
}

}  // namespace skia
