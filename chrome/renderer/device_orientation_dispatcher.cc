// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/device_orientation_dispatcher.h"

#include "chrome/common/render_messages_params.h"
#include "chrome/renderer/render_view.h"
#include "third_party/WebKit/WebKit/chromium/public/WebDeviceOrientation.h"
#include "third_party/WebKit/WebKit/chromium/public/WebDeviceOrientationController.h"

DeviceOrientationDispatcher::DeviceOrientationDispatcher(
    RenderView* render_view)
    : render_view_(render_view),
      controller_(NULL),
      started_(false) {
}

DeviceOrientationDispatcher::~DeviceOrientationDispatcher() {
  if (started_)
    stopUpdating();
}

bool DeviceOrientationDispatcher::OnMessageReceived(const IPC::Message& msg) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(DeviceOrientationDispatcher, msg)
    IPC_MESSAGE_HANDLER(ViewMsg_DeviceOrientationUpdated,
                        OnDeviceOrientationUpdated)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void DeviceOrientationDispatcher::setController(
    WebKit::WebDeviceOrientationController* controller) {
  controller_.reset(controller);
}

void DeviceOrientationDispatcher::startUpdating() {
  render_view_->Send(new ViewHostMsg_DeviceOrientation_StartUpdating(
      render_view_->routing_id()));
  started_ = true;
}

void DeviceOrientationDispatcher::stopUpdating() {
  render_view_->Send(new ViewHostMsg_DeviceOrientation_StopUpdating(
      render_view_->routing_id()));
  started_ = false;
}

WebKit::WebDeviceOrientation DeviceOrientationDispatcher::lastOrientation()
    const {
  if (!last_orientation_.get())
    return WebKit::WebDeviceOrientation::nullOrientation();

  return *last_orientation_;
}

namespace {
bool OrientationsEqual(const ViewMsg_DeviceOrientationUpdated_Params& a,
                       WebKit::WebDeviceOrientation* b) {
  if (a.can_provide_alpha != b->canProvideAlpha())
    return false;
  if (a.can_provide_alpha && a.alpha != b->alpha())
    return false;
  if (a.can_provide_beta != b->canProvideBeta())
    return false;
  if (a.can_provide_beta && a.beta != b->beta())
    return false;
  if (a.can_provide_gamma != b->canProvideGamma())
    return false;
  if (a.can_provide_gamma && a.gamma != b->gamma())
    return false;

  return true;
}
}  // namespace

void DeviceOrientationDispatcher::OnDeviceOrientationUpdated(
    const ViewMsg_DeviceOrientationUpdated_Params& p) {

  if (last_orientation_.get() && OrientationsEqual(p, last_orientation_.get()))
    return;

  last_orientation_.reset(new WebKit::WebDeviceOrientation(p.can_provide_alpha,
                                                           p.alpha,
                                                           p.can_provide_beta,
                                                           p.beta,
                                                           p.can_provide_gamma,
                                                           p.gamma));

  controller_->didChangeDeviceOrientation(*last_orientation_);
}
