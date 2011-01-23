// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "views/view.h"

#include <gtk/gtk.h>

#include "base/logging.h"
#include "views/views_delegate.h"

namespace views {

// static
int View::GetDoubleClickTimeMS() {
   return 0;
}

int View::GetMenuShowDelay() {
  return 0;
}

void View::NotifyAccessibilityEvent(AccessibilityTypes::Event event_type,
    bool send_native_event) {

}

ViewAccessibility* View::GetViewAccessibility() {
  return NULL;
}

int View::GetHorizontalDragThreshold() {
 return 0;
}

int View::GetVerticalDragThreshold() {

  return 0;
}

}  // namespace views

