// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This implements the JavaScript class entrypoint for the plugin instance.
// The Javascript API is defined as follows.
//
// interface ChromotingScriptableObject {
//
//   // Connection status.
//   readonly attribute unsigned short connection_status;

//   // Constants for connection status.
//   const unsigned short STATUS_UNKNOWN = 0;
//   const unsigned short STATUS_CONNECTING = 1;
//   const unsigned short STATUS_INITIALIZING = 2;
//   const unsigned short STATUS_CONNECTED = 3;
//   const unsigned short STATUS_CLOSED = 4;
//   const unsigned short STATUS_FAILED = 5;
//
//   // Connection quality.
//   readonly attribute unsigned short connection_quality;
//   // Constants for connection quality
//   const unsigned short QUALITY_UNKNOWN = 0;
//   const unsigned short QUALITY_GOOD = 1;
//   const unsigned short QUALITY_BAD = 2;
//
//   // JS callback function so we can signal the JS UI when the connection
//   // status has been updated.
//   attribute Function connectionInfoUpdate;
//
//   // This function is called with a callback function as argument. The
//   // signature of this function is:
//   // function login(username, password);
//   //
//   // The provided callback function should be called when username and
//   // password is available, e.g. collected by a login prompt.
//   //
//   // This function will be called multiple times until login was successful
//   // or the maximum number of login attempts has been reached. In the
//   // later case |connection_status| is changed to STATUS_FAILED.
//   attribute Function loginChallenge;
//
//   // Methods on the object.
//   void connect(string username, string host_jid, string auth_token);
//   void disconnect();
// }

#ifndef REMOTING_CLIENT_PLUGIN_CHROMOTING_SCRIPTABLE_OBJECT_H_
#define REMOTING_CLIENT_PLUGIN_CHROMOTING_SCRIPTABLE_OBJECT_H_

#include <map>
#include <string>
#include <vector>

#include "ppapi/cpp/dev/scriptable_object_deprecated.h"
#include "ppapi/cpp/var.h"

namespace remoting {

class ChromotingInstance;

extern const char kStatusAttribute[];

enum ConnectionStatus {
  STATUS_UNKNOWN = 0,
  STATUS_CONNECTING,
  STATUS_INITIALIZING,
  STATUS_CONNECTED,
  STATUS_CLOSED,
  STATUS_FAILED,
};

extern const char kQualityAttribute[];

enum ConnectionQuality {
  QUALITY_UNKNOWN = 0,
  QUALITY_GOOD,
  QUALITY_BAD,
};

class ChromotingScriptableObject : public pp::deprecated::ScriptableObject {
 public:
  explicit ChromotingScriptableObject(ChromotingInstance* instance);
  virtual ~ChromotingScriptableObject();

  virtual void Init();

  // Override the ScriptableObject functions.
  virtual bool HasProperty(const pp::Var& name, pp::Var* exception);
  virtual bool HasMethod(const pp::Var& name, pp::Var* exception);
  virtual pp::Var GetProperty(const pp::Var& name, pp::Var* exception);
  virtual void GetAllPropertyNames(std::vector<pp::Var>* properties,
                                   pp::Var* exception);
  virtual void SetProperty(const pp::Var& name,
                           const pp::Var& value,
                           pp::Var* exception);
  virtual pp::Var Call(const pp::Var& method_name,
                       const std::vector<pp::Var>& args,
                       pp::Var* exception);

  void SetConnectionInfo(ConnectionStatus status, ConnectionQuality quality);

 private:
  typedef std::map<std::string, int> PropertyNameMap;
  typedef pp::Var (ChromotingScriptableObject::*MethodHandler)(
      const std::vector<pp::Var>& args, pp::Var* exception);
  struct PropertyDescriptor {
    explicit PropertyDescriptor(const std::string& n, pp::Var a)
        : name(n), attribute(a), method(NULL) {
    }

    explicit PropertyDescriptor(const std::string& n, MethodHandler m)
        : name(n), method(m) {
    }

    enum Type {
      ATTRIBUTE,
      METHOD,
    } type;

    std::string name;
    pp::Var attribute;
    MethodHandler method;
  };


  // Routines to add new attribute, method properties.
  void AddAttribute(const std::string& name, pp::Var attribute);
  void AddMethod(const std::string& name, MethodHandler handler);

  // This should be called to signal the JS code that the connection status has
  // changed.
  void SignalConnectionInfoChange();

  // This should be called to signal JS code to provide login information.
  void SignalLoginChallenge();

  pp::Var DoConnect(const std::vector<pp::Var>& args, pp::Var* exception);
  pp::Var DoDisconnect(const std::vector<pp::Var>& args, pp::Var* exception);

  // This method is called by JS to provide login information. Note that this
  // method is provided as a callback.
  pp::Var DoLogin(const std::vector<pp::Var>& args, pp::Var* exception);

  PropertyNameMap property_names_;
  std::vector<PropertyDescriptor> properties_;

  ChromotingInstance* instance_;
};

}  // namespace remoting

#endif  // REMOTING_CLIENT_PLUGIN_CHROMOTING_SCRIPTABLE_OBJECT_H_
