// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_SERVICE_REMOTING_CHROMOTING_HOST_MANAGER_H_
#define CHROME_SERVICE_REMOTING_CHROMOTING_HOST_MANAGER_H_

#include <string>

#include "base/ref_counted.h"
#include "base/scoped_ptr.h"
#include "remoting/host/chromoting_host.h"
#include "remoting/host/chromoting_host_context.h"
#include "remoting/host/host_config.h"

namespace base {
class MessageLoopProxy;
}  // namespace base

namespace remoting {

struct ChromotingHostInfo;

// ChromotingHostManager manages chromoting host. It loads config and updates
// config when necessary, and starts and stops the chromoting host.
class ChromotingHostManager
    : public base::RefCountedThreadSafe<ChromotingHostManager> {
 public:

  // Interface for observer that is notified about the host being
  // enabled/disabled. Observer is specified in the constructor.
  class Observer {
   public:
    virtual ~Observer() {}
    virtual void OnChromotingHostEnabled() {}
    virtual void OnChromotingHostDisabled() {}
  };

  // Caller keeps ownership of |observer|. |observer| must not be
  // destroyed while this object exists.
  ChromotingHostManager(Observer* observer);

  void Initialize(base::MessageLoopProxy* file_message_loop);
  void Teardown();

  // Return the reference to the chromoting host only if it has started.
  remoting::ChromotingHost* GetChromotingHost() { return chromoting_host_; }

  // Updates credentials used for XMPP connection.
  void SetCredentials(const std::string& login, const std::string& token);

  bool IsEnabled();

  // Start running the chromoting host asynchronously.
  void Enable();

  // Stop chromoting host. The shutdown process will happen asynchronously.
  void Disable();

  void GetHostInfo(ChromotingHostInfo* host_info);

 private:
  bool IsConfigInitialized();
  void InitializeConfig();

  void SetEnabled(bool enabled);
  void Start();
  void Stop();

  void OnShutdown();

  Observer* observer_;

  scoped_refptr<remoting::MutableHostConfig> chromoting_config_;
  scoped_ptr<remoting::ChromotingHostContext> chromoting_context_;
  scoped_refptr<remoting::ChromotingHost> chromoting_host_;
};

}  // namespace remoting

#endif  // CHROME_SERVICE_REMOTING_CHROMOTING_HOST_MANAGER_H_
