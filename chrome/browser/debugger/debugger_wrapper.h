// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Include this file if you need to access the Debugger outside of the debugger
// project.  Don't include debugger.h directly.  If there's functionality from
// Debugger needed, add new wrapper methods to this file.

#ifndef CHROME_BROWSER_DEBUGGER_DEBUGGER_WRAPPER_H_
#define CHROME_BROWSER_DEBUGGER_DEBUGGER_WRAPPER_H_
#pragma once

#include "base/basictypes.h"
#include "base/ref_counted.h"

class DebuggerHost;
class DevToolsHttpProtocolHandler;
class DevToolsProtocolHandler;
class DevToolsRemoteListenSocket;

class DebuggerWrapper : public base::RefCountedThreadSafe<DebuggerWrapper> {
 public:
  DebuggerWrapper(int port, bool useHttp);

 private:
  friend class base::RefCountedThreadSafe<DebuggerWrapper>;

  virtual ~DebuggerWrapper();

  scoped_refptr<DevToolsProtocolHandler> proto_handler_;
  scoped_refptr<DevToolsHttpProtocolHandler> http_handler_;
};

#endif  // CHROME_BROWSER_DEBUGGER_DEBUGGER_WRAPPER_H_
