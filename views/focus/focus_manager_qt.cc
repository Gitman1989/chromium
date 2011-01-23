// Copyright (c) 2006-2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "views/focus/focus_manager.h"

#include "base/logging.h"
#include "views/widget/widget_qt.h"
#include "views/window/window_qt.h"

namespace views {

void FocusManager::ClearNativeFocus() {
}

void FocusManager::FocusNativeView(gfx::NativeView native_view) {
}

// static
FocusManager* FocusManager::GetFocusManagerForNativeView(
    gfx::NativeView native_view) {
  return NULL;
}

// static
FocusManager* FocusManager::GetFocusManagerForNativeWindow(
    gfx::NativeWindow native_window) {
  return NULL;
}

}  // namespace views

