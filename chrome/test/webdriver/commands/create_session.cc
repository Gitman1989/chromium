// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/webdriver/commands/create_session.h"

#include <sstream>
#include <string>

#include "base/values.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/common/chrome_constants.h"

namespace webdriver {

void CreateSession::ExecutePost(Response* const response) {
  SessionManager* session_manager = SessionManager::GetInstance();
  std::string session_id;

  if (!session_manager->Create(&session_id)) {
    response->set_status(kUnknownError);
    response->set_value(Value::CreateStringValue("Failed to create session"));
    return;
  }

  VLOG(1) << "Created session " << session_id;
  std::ostringstream stream;
  stream << "http://" << session_manager->GetIPAddress() << "/session/"
         << session_id;
  response->set_status(kSeeOther);
  response->set_value(Value::CreateStringValue(stream.str()));
}

}  // namespace webdriver
