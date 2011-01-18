// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_WORKER_NATIVEWEBWORKER_IMPL_H_
#define CHROME_WORKER_NATIVEWEBWORKER_IMPL_H_
#pragma once

#include "base/basictypes.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebWorker.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebWorkerClient.h"


// Forward declaration for the listener thread pointer.
class NativeWebWorkerListenerThread;

// This class is used by the worker process code to talk to the Native Client
// worker implementation.
class NativeWebWorkerImpl : public WebKit::WebWorker {
 public:
  explicit NativeWebWorkerImpl(WebKit::WebWorkerClient* client);
  virtual ~NativeWebWorkerImpl();

  static WebWorker* create(WebKit::WebWorkerClient* client);

  // WebWorker implementation.
  virtual void startWorkerContext(const WebKit::WebURL& script_url,
                                  const WebKit::WebString& user_agent,
                                  const WebKit::WebString& source_code);
  virtual void terminateWorkerContext();
  virtual void postMessageToWorkerContext(
      const WebKit::WebString& message,
      const WebKit::WebMessagePortChannelArray& channels);
  virtual void workerObjectDestroyed();
  virtual void clientDestroyed();

 private:
  WebKit::WebWorkerClient* client_;
  struct NaClApp* nap_;
  struct NaClSrpcChannel* channel_;
  NativeWebWorkerListenerThread* upcall_thread_;
  struct NaClDesc* descs_[2];

  DISALLOW_COPY_AND_ASSIGN(NativeWebWorkerImpl);
};

#endif  // CHROME_WORKER_NATIVEWEBWORKER_IMPL_H_
