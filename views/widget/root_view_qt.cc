// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "views/widget/root_view.h"



#include "base/logging.h"
#include "gfx/canvas_skia_paint.h"
#include "views/widget/widget_qt.h"

namespace views {

void RootView::OnPaint(GdkEventExpose* event) {

}

void RootView::StartDragForViewFromMouseEvent(
    View* view,
    const OSExchangeData& data,
    int operation) {

}

}
