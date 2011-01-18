// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/client/plugin/chromoting_scriptable_object.h"

#include "base/logging.h"
#include "ppapi/cpp/var.h"
#include "remoting/client/client_config.h"
#include "remoting/client/plugin/chromoting_instance.h"

using pp::Var;

namespace remoting {

const char kStatusAttribute[] = "status";
const char kQualityAttribute[] = "quality";
const char kConnectionInfoUpdate[] = "connectionInfoUpdate";
const char kLoginChallenge[] = "loginChallenge";

ChromotingScriptableObject::ChromotingScriptableObject(
    ChromotingInstance* instance)
    : instance_(instance) {
}

ChromotingScriptableObject::~ChromotingScriptableObject() {
}

void ChromotingScriptableObject::Init() {
  // Property addition order should match the interface description at the
  // top of chromoting_scriptable_object.h.

  // Connection status.
  AddAttribute(kStatusAttribute, Var(STATUS_UNKNOWN));

  // Connection status values.
  AddAttribute("STATUS_UNKNOWN", Var(STATUS_UNKNOWN));
  AddAttribute("STATUS_CONNECTING", Var(STATUS_CONNECTING));
  AddAttribute("STATUS_INITIALIZING", Var(STATUS_INITIALIZING));
  AddAttribute("STATUS_CONNECTED", Var(STATUS_CONNECTED));
  AddAttribute("STATUS_CLOSED", Var(STATUS_CLOSED));
  AddAttribute("STATUS_FAILED", Var(STATUS_FAILED));

  // Connection quality.
  AddAttribute(kQualityAttribute, Var(QUALITY_UNKNOWN));

  // Connection quality values.
  AddAttribute("QUALITY_UNKNOWN", Var(QUALITY_UNKNOWN));
  AddAttribute("QUALITY_GOOD", Var(QUALITY_GOOD));
  AddAttribute("QUALITY_BAD", Var(QUALITY_BAD));

  AddAttribute(kConnectionInfoUpdate, Var());
  AddAttribute(kLoginChallenge, Var());

  AddMethod("connect", &ChromotingScriptableObject::DoConnect);
  AddMethod("disconnect", &ChromotingScriptableObject::DoDisconnect);
}

bool ChromotingScriptableObject::HasProperty(const Var& name, Var* exception) {
  // TODO(ajwong): Check if all these name.is_string() sentinels are required.
  if (!name.is_string()) {
    *exception = Var("HasProperty expects a string for the name.");
    return false;
  }

  PropertyNameMap::const_iterator iter = property_names_.find(name.AsString());
  if (iter == property_names_.end()) {
    return false;
  }

  // TODO(ajwong): Investigate why ARM build breaks if you do:
  //    properties_[iter->second].method == NULL;
  // Somehow the ARM compiler is thinking that the above is using NULL as an
  // arithmetic expression.
  return properties_[iter->second].method == 0;
}

bool ChromotingScriptableObject::HasMethod(const Var& name, Var* exception) {
  // TODO(ajwong): Check if all these name.is_string() sentinels are required.
  if (!name.is_string()) {
    *exception = Var("HasMethod expects a string for the name.");
    return false;
  }

  PropertyNameMap::const_iterator iter = property_names_.find(name.AsString());
  if (iter == property_names_.end()) {
    return false;
  }

  // See comment from HasProperty about why to use 0 instead of NULL here.
  return properties_[iter->second].method != 0;
}

Var ChromotingScriptableObject::GetProperty(const Var& name, Var* exception) {
  // TODO(ajwong): Check if all these name.is_string() sentinels are required.
  if (!name.is_string()) {
    *exception = Var("GetProperty expects a string for the name.");
    return Var();
  }

  PropertyNameMap::const_iterator iter = property_names_.find(name.AsString());

  // No property found.
  if (iter == property_names_.end()) {
    return ScriptableObject::GetProperty(name, exception);
  }

  // TODO(ajwong): This incorrectly return a null object if a function
  // property is requested.
  return properties_[iter->second].attribute;
}

void ChromotingScriptableObject::GetAllPropertyNames(
    std::vector<Var>* properties,
    Var* exception) {
  for (size_t i = 0; i < properties_.size(); i++) {
    properties->push_back(Var(properties_[i].name));
  }
}

void ChromotingScriptableObject::SetProperty(const Var& name,
                                             const Var& value,
                                             Var* exception) {
  // TODO(ajwong): Check if all these name.is_string() sentinels are required.   120   // No externally settable properties for Chromoting.
  if (!name.is_string()) {
    *exception = Var("SetProperty expects a string for the name.");
    return;
  }

  // Allow writing to connectionInfoUpdate or loginChallenge.  See top of
  // chromoting_scriptable_object.h for the object interface definition.
  std::string property_name = name.AsString();
  if (property_name != kConnectionInfoUpdate &&
      property_name != kLoginChallenge) {
    *exception =
        Var("Cannot set property " + property_name + " on this object.");
    return;
  }

  // Since we're whitelisting the propertie that are settable above, we can
  // assume that the property exists in the map.
  properties_[property_names_[property_name]].attribute = value;
}

Var ChromotingScriptableObject::Call(const Var& method_name,
                                     const std::vector<Var>& args,
                                     Var* exception) {
  PropertyNameMap::const_iterator iter =
      property_names_.find(method_name.AsString());
  if (iter == property_names_.end()) {
    return pp::deprecated::ScriptableObject::Call(method_name, args, exception);
  }

  return (this->*(properties_[iter->second].method))(args, exception);
}

void ChromotingScriptableObject::SetConnectionInfo(ConnectionStatus status,
                                                   ConnectionQuality quality) {
  int status_index = property_names_[kStatusAttribute];
  int quality_index = property_names_[kQualityAttribute];

  if (properties_[status_index].attribute.AsInt() != status ||
      properties_[quality_index].attribute.AsInt() != quality) {
    // Update the connection state properties..
    properties_[status_index].attribute = Var(status);
    properties_[quality_index].attribute = Var(quality);

    // Signal the Chromoting Tab UI to get the update connection state values.
    SignalConnectionInfoChange();
  }
}

void ChromotingScriptableObject::AddAttribute(const std::string& name,
                                              Var attribute) {
  property_names_[name] = properties_.size();
  properties_.push_back(PropertyDescriptor(name, attribute));
}

void ChromotingScriptableObject::AddMethod(const std::string& name,
                                           MethodHandler handler) {
  property_names_[name] = properties_.size();
  properties_.push_back(PropertyDescriptor(name, handler));
}

void ChromotingScriptableObject::SignalConnectionInfoChange() {
  pp::Var exception;

  // The JavaScript callback function is the 'callback' property on the
  // 'kConnectionInfoUpdate' object.
  Var cb = GetProperty(Var(kConnectionInfoUpdate), &exception);

  // Var() means call the object directly as a function rather than calling
  // a method in the object.
  cb.Call(Var(), 0, NULL, &exception);

  if (!exception.is_undefined()) {
    LOG(WARNING) << "Exception when invoking JS callback"
                 << exception.AsString();
  }
}

void ChromotingScriptableObject::SignalLoginChallenge() {
  pp::Var exception;

  Var fun = GetProperty(Var(kLoginChallenge), &exception);

  // Calls the loginChallenge() function with a callback.
  fun.Call(Var(), MethodHandler(&ChromotingScriptableObject::DoLogin),
           &exception);

  if (!exception.is_undefined()) {
    LOG(WARNING) << "Exception when calling JS callback"
                 << exception.AsString();
  }
}

pp::Var ChromotingScriptableObject::DoConnect(const std::vector<Var>& args,
                                              Var* exception) {
  if (args.size() != 3) {
    *exception = Var("Usage: connect(username, host_jid, auth_token)");
    return Var();
  }

  ClientConfig config;

  if (!args[0].is_string()) {
    *exception = Var("The username must be a string.");
    return Var();
  }
  config.username = args[0].AsString();

  if (!args[1].is_string()) {
    *exception = Var("The host_jid must be a string.");
    return Var();
  }
  config.host_jid = args[1].AsString();

  if (!args[2].is_string()) {
    *exception = Var("The auth_token must be a string.");
    return Var();
  }
  config.auth_token = args[2].AsString();

  instance_->Connect(config);

  return Var();
}

pp::Var ChromotingScriptableObject::DoDisconnect(const std::vector<Var>& args,
                                                 Var* exception) {
  instance_->Disconnect();
  return Var();
}

pp::Var ChromotingScriptableObject::DoLogin(const std::vector<pp::Var>& args,
                                            pp::Var* exception) {
  NOTIMPLEMENTED();
  return Var();
}

}  // namespace remoting
