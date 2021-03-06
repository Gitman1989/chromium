// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/devtools_agent.h"

#include <map>

#include "base/command_line.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/render_messages.h"
#include "chrome/renderer/devtools_agent_filter.h"
#include "chrome/renderer/render_view.h"
#include "grit/webkit_chromium_resources.h"
#include "third_party/WebKit/WebKit/chromium/public/WebDevToolsAgent.h"
#include "third_party/WebKit/WebKit/chromium/public/WebPoint.h"
#include "third_party/WebKit/WebKit/chromium/public/WebString.h"
#include "third_party/WebKit/WebKit/chromium/public/WebView.h"
#include "webkit/glue/webkit_glue.h"

using WebKit::WebDevToolsAgent;
using WebKit::WebDevToolsAgentClient;
using WebKit::WebPoint;
using WebKit::WebString;
using WebKit::WebCString;
using WebKit::WebVector;
using WebKit::WebView;

namespace {

class WebKitClientMessageLoopImpl
    : public WebDevToolsAgentClient::WebKitClientMessageLoop {
 public:
  WebKitClientMessageLoopImpl() : message_loop_(MessageLoop::current()) { }
  virtual ~WebKitClientMessageLoopImpl() {
    message_loop_ = NULL;
  }
  virtual void run() {
    bool old_state = message_loop_->NestableTasksAllowed();
    message_loop_->SetNestableTasksAllowed(true);
    message_loop_->Run();
    message_loop_->SetNestableTasksAllowed(old_state);
  }
  virtual void quitNow() {
    message_loop_->QuitNow();
  }
 private:
  MessageLoop* message_loop_;
};

} //  namespace

// static
std::map<int, DevToolsAgent*> DevToolsAgent::agent_for_routing_id_;

DevToolsAgent::DevToolsAgent(int routing_id, RenderView* render_view)
    : routing_id_(routing_id),
      render_view_(render_view) {
  agent_for_routing_id_[routing_id] = this;

  CommandLine* cmd = CommandLine::ForCurrentProcess();
  expose_v8_debugger_protocol_ =cmd->HasSwitch(switches::kRemoteShellPort);
}

DevToolsAgent::~DevToolsAgent() {
  agent_for_routing_id_.erase(routing_id_);
}

void DevToolsAgent::OnNavigate() {
  WebDevToolsAgent* web_agent = GetWebAgent();
  if (web_agent) {
    web_agent->didNavigate();
  }
}

// Called on the Renderer thread.
bool DevToolsAgent::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(DevToolsAgent, message)
    IPC_MESSAGE_HANDLER(DevToolsAgentMsg_Attach, OnAttach)
    IPC_MESSAGE_HANDLER(DevToolsAgentMsg_Detach, OnDetach)
    IPC_MESSAGE_HANDLER(DevToolsAgentMsg_FrontendLoaded, OnFrontendLoaded)
    IPC_MESSAGE_HANDLER(DevToolsAgentMsg_DispatchOnInspectorBackend,
                        OnDispatchOnInspectorBackend)
    IPC_MESSAGE_HANDLER(DevToolsAgentMsg_InspectElement, OnInspectElement)
    IPC_MESSAGE_HANDLER(DevToolsAgentMsg_SetApuAgentEnabled,
                        OnSetApuAgentEnabled)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void DevToolsAgent::sendMessageToInspectorFrontend(
    const WebKit::WebString& message) {
  IPC::Message* m = new ViewHostMsg_ForwardToDevToolsClient(
      routing_id_,
      DevToolsClientMsg_DispatchOnInspectorFrontend(message.utf8()));
  render_view_->Send(m);
}

void DevToolsAgent::sendDebuggerOutput(const WebKit::WebString& data) {
  IPC::Message* m = new ViewHostMsg_ForwardToDevToolsClient(
      routing_id_,
      DevToolsClientMsg_DebuggerOutput(data.utf8()));
  render_view_->Send(m);
}

void DevToolsAgent::sendDispatchToAPU(const WebKit::WebString& data) {
  IPC::Message* m = new ViewHostMsg_ForwardToDevToolsClient(
      routing_id_,
      DevToolsClientMsg_DispatchToAPU(data.utf8()));
  render_view_->Send(m);
}

int DevToolsAgent::hostIdentifier() {
  return routing_id_;
}

void DevToolsAgent::runtimeFeatureStateChanged(
    const WebKit::WebString& feature,
    bool enabled) {
  render_view_->Send(new ViewHostMsg_DevToolsRuntimePropertyChanged(
      routing_id_,
      feature.utf8(),
      enabled ? "true" : "false"));
}

void DevToolsAgent::runtimePropertyChanged(
    const WebKit::WebString& name,
    const WebKit::WebString& value) {
  render_view_->Send(new ViewHostMsg_DevToolsRuntimePropertyChanged(
      routing_id_,
      name.utf8(),
      value.utf8()));
}

WebCString DevToolsAgent::debuggerScriptSource() {
  base::StringPiece debuggerScriptjs =
      webkit_glue::GetDataResource(IDR_DEVTOOLS_DEBUGGER_SCRIPT_JS);
  return WebCString(debuggerScriptjs.data(), debuggerScriptjs.length());
}

WebKit::WebDevToolsAgentClient::WebKitClientMessageLoop*
    DevToolsAgent::createClientMessageLoop() {
  return new WebKitClientMessageLoopImpl();
}

bool DevToolsAgent::exposeV8DebuggerProtocol() {
  return expose_v8_debugger_protocol_;
}


// static
DevToolsAgent* DevToolsAgent::FromHostId(int host_id) {
  std::map<int, DevToolsAgent*>::iterator it =
      agent_for_routing_id_.find(host_id);
  if (it != agent_for_routing_id_.end()) {
    return it->second;
  }
  return NULL;
}

void DevToolsAgent::OnAttach(
    const DevToolsRuntimeProperties& runtime_properties) {
  WebDevToolsAgent* web_agent = GetWebAgent();
  if (web_agent) {
    web_agent->attach();
    for (DevToolsRuntimeProperties::const_iterator it =
             runtime_properties.begin();
         it != runtime_properties.end(); ++it) {
      web_agent->setRuntimeProperty(WebString::fromUTF8(it->first),
                                    WebString::fromUTF8(it->second));
    }
  }
}

void DevToolsAgent::OnDetach() {
  WebDevToolsAgent* web_agent = GetWebAgent();
  if (web_agent)
    web_agent->detach();
}

void DevToolsAgent::OnFrontendLoaded() {
  WebDevToolsAgent* web_agent = GetWebAgent();
  if (web_agent)
    web_agent->frontendLoaded();
}

void DevToolsAgent::OnDispatchOnInspectorBackend(const std::string& message) {
  WebDevToolsAgent* web_agent = GetWebAgent();
  if (web_agent)
    web_agent->dispatchOnInspectorBackend(WebString::fromUTF8(message));
}

void DevToolsAgent::OnInspectElement(int x, int y) {
  WebDevToolsAgent* web_agent = GetWebAgent();
  if (web_agent) {
    web_agent->attach();
    web_agent->inspectElementAt(WebPoint(x, y));
  }
}

void DevToolsAgent::OnSetApuAgentEnabled(bool enabled) {
  WebDevToolsAgent* web_agent = GetWebAgent();
  if (web_agent)
    web_agent->setRuntimeProperty("apu-agent", enabled ?
        WebString::fromUTF8("true") : WebString::fromUTF8("false"));
}

WebDevToolsAgent* DevToolsAgent::GetWebAgent() {
  WebView* web_view = render_view_->webview();
  if (!web_view)
    return NULL;
  return web_view->devToolsAgent();
}
