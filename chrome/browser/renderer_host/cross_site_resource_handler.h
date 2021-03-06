// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RENDERER_HOST_CROSS_SITE_RESOURCE_HANDLER_H_
#define CHROME_BROWSER_RENDERER_HOST_CROSS_SITE_RESOURCE_HANDLER_H_
#pragma once

#include "chrome/browser/renderer_host/resource_handler.h"
#include "net/url_request/url_request_status.h"

class ResourceDispatcherHost;
struct GlobalRequestID;

// Ensures that cross-site responses are delayed until the onunload handler of
// the previous page is allowed to run.  This handler wraps an
// AsyncEventHandler, and it sits inside SafeBrowsing and Buffered event
// handlers.  This is important, so that it can intercept OnResponseStarted
// after we determine that a response is safe and not a download.
class CrossSiteResourceHandler : public ResourceHandler {
 public:
  CrossSiteResourceHandler(ResourceHandler* handler,
                           int render_process_host_id,
                           int render_view_id,
                           ResourceDispatcherHost* resource_dispatcher_host);

  // ResourceHandler implementation:
  virtual bool OnUploadProgress(int request_id, uint64 position, uint64 size);
  virtual bool OnRequestRedirected(int request_id, const GURL& new_url,
                                   ResourceResponse* response, bool* defer);
  virtual bool OnResponseStarted(int request_id,
                                 ResourceResponse* response);
  virtual bool OnWillStart(int request_id, const GURL& url, bool* defer);
  virtual bool OnWillRead(int request_id, net::IOBuffer** buf, int* buf_size,
                          int min_size);
  virtual bool OnReadCompleted(int request_id, int* bytes_read);
  virtual bool OnResponseCompleted(int request_id,
                                   const URLRequestStatus& status,
                                   const std::string& security_info);
  virtual void OnRequestClosed();

  // We can now send the response to the new renderer, which will cause
  // TabContents to swap in the new renderer and destroy the old one.
  void ResumeResponse();

 private:
  virtual ~CrossSiteResourceHandler();

  // Prepare to render the cross-site response in a new RenderViewHost, by
  // telling the old RenderViewHost to run its onunload handler.
  void StartCrossSiteTransition(
      int request_id,
      ResourceResponse* response,
      const GlobalRequestID& global_id);

  scoped_refptr<ResourceHandler> next_handler_;
  int render_process_host_id_;
  int render_view_id_;
  bool has_started_response_;
  bool in_cross_site_transition_;
  int request_id_;
  bool completed_during_transition_;
  URLRequestStatus completed_status_;
  std::string completed_security_info_;
  ResourceResponse* response_;
  ResourceDispatcherHost* rdh_;

  DISALLOW_COPY_AND_ASSIGN(CrossSiteResourceHandler);
};

#endif  // CHROME_BROWSER_RENDERER_HOST_CROSS_SITE_RESOURCE_HANDLER_H_
