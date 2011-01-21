// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "printing/printed_document.h"

#include "base/file_util.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "printing/page_number.h"
#include "printing/printed_page.h"

namespace printing {

void PrintedDocument::RenderPrintedPage(
    const PrintedPage& page, gfx::NativeDrawingContext context) const {
#ifndef NDEBUG
  {
    // Make sure the page is from our list.
    base::AutoLock lock(lock_);
    DCHECK(&page == mutable_.pages_.find(page.page_number() - 1)->second.get());
  }
#endif

  DCHECK(context);

  NOTIMPLEMENTED();
}

void PrintedDocument::DrawHeaderFooter(gfx::NativeDrawingContext context,
                                       std::wstring text,
                                       gfx::Rect bounds) const {
  NOTIMPLEMENTED();
}

}  // namespace printing
