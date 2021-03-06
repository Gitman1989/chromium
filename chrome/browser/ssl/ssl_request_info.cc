// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/ssl_request_info.h"

SSLRequestInfo::SSLRequestInfo(const GURL& url,
                               ResourceType::Type resource_type,
                               const std::string& frame_origin,
                               const std::string& main_frame_origin,
                               int child_id,
                               int ssl_cert_id,
                               int ssl_cert_status)
    : url_(url),
      resource_type_(resource_type),
      frame_origin_(frame_origin),
      main_frame_origin_(main_frame_origin),
      child_id_(child_id),
      ssl_cert_id_(ssl_cert_id),
      ssl_cert_status_(ssl_cert_status) {
}

SSLRequestInfo::~SSLRequestInfo() {}
