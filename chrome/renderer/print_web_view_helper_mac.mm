// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/print_web_view_helper.h"

#import <AppKit/AppKit.h>

#include "app/l10n_util.h"
#include "base/logging.h"
#include "chrome/common/render_messages.h"
#include "chrome/common/render_messages_params.h"
#include "chrome/renderer/render_view.h"
#include "grit/generated_resources.h"
#include "printing/native_metafile.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebFrame.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebCanvas.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebRect.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebSize.h"

using WebKit::WebFrame;
using WebKit::WebCanvas;
using WebKit::WebRect;
using WebKit::WebSize;

void PrintWebViewHelper::PrintPage(const ViewMsg_PrintPage_Params& params,
                                   const gfx::Size& canvas_size,
                                   WebFrame* frame) {
  printing::NativeMetafile metafile;
  if (!metafile.Init())
    return;

  float scale_factor = frame->getPrintPageShrink(params.page_number);
  double width = params.params.printable_size.width();
  double height = params.params.printable_size.height();
  int page_number = params.page_number;

  // Render page for printing.
  gfx::Point origin(0.0f, 0.0f);
  RenderPage(params.params.printable_size, origin, scale_factor, page_number,
      frame, &metafile);

  metafile.Close();

  double margin_left = params.params.margin_left;
  double margin_top = params.params.margin_top;

  // Get the size of the compiled metafile.
  ViewHostMsg_DidPrintPage_Params page_params;
  page_params.data_size = 0;
  page_params.page_number = page_number;
  page_params.document_cookie = params.params.document_cookie;
  page_params.actual_shrink = scale_factor;

  page_params.page_size = params.params.page_size;
  page_params.content_area = gfx::Rect(margin_left, margin_top, width, height);

  // Ask the browser to create the shared memory for us.
  if (!CopyMetafileDataToSharedMem(&metafile,
          &(page_params.metafile_data_handle))) {
    NOTREACHED();
    return;
  }

  page_params.data_size = metafile.GetDataSize();
  Send(new ViewHostMsg_DidPrintPage(routing_id(), page_params));
}

void PrintWebViewHelper::CreatePreviewDocument(
    const ViewMsg_PrintPages_Params& params,
    WebFrame* frame,
    ViewHostMsg_DidPreviewDocument_Params* print_params) {
  ViewMsg_Print_Params printParams = params.params;
  UpdatePrintableSizeInPrintParameters(frame, NULL, &printParams);

  PrepareFrameAndViewForPrint prep_frame_view(printParams,
                                              frame, NULL, frame->view());
  int page_count = prep_frame_view.GetExpectedPageCount();

  if (!page_count)
    return;

  float scale_factor = frame->getPrintPageShrink(0);
  double originX = printParams.margin_left;
  double originY = printParams.margin_top;

  printing::NativeMetafile metafile;
  if (!metafile.Init())
    return;

  gfx::Point origin(originX, originY);

  if (params.pages.empty()) {
    for (int i = 0; i < page_count; ++i) {
      RenderPage(printParams.page_size, origin, scale_factor, i, frame,
          &metafile);
    }
  } else {
    for (size_t i = 0; i < params.pages.size(); ++i) {
      if (params.pages[i] >= page_count)
        break;
      RenderPage(printParams.page_size, origin, scale_factor,
          static_cast<int>(params.pages[i]), frame, &metafile);
    }
  }
  metafile.Close();
  // Ask the browser to create the shared memory for us.
  if (!CopyMetafileDataToSharedMem(&metafile,
          &(print_params->metafile_data_handle))) {
    NOTREACHED();
    return;
  }
  print_params->document_cookie = params.params.document_cookie;
  print_params->data_size = metafile.GetDataSize();
}

void PrintWebViewHelper::RenderPage(
    const gfx::Size& page_size, const gfx::Point& content_origin,
    const float& scale_factor, int page_number, WebFrame* frame,
    printing::NativeMetafile* metafile) {
  CGContextRef context = metafile->StartPage(page_size, content_origin,
                                             scale_factor);
  DCHECK(context);

  // printPage can create autoreleased references to |context|. PDF contexts
  // don't write all their data until they are destroyed, so we need to make
  // certain that there are no lingering references.
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
  frame->printPage(page_number, context);
  [pool release];

  // Done printing. Close the device context to retrieve the compiled metafile.
  metafile->FinishPage();
}
