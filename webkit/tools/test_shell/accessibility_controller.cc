// Copyright (c) 2006-2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "webkit/tools/test_shell/accessibility_controller.h"

#include "base/logging.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebAccessibilityCache.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebAccessibilityObject.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebFrame.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebView.h"
#include "webkit/tools/test_shell/test_shell.h"

using WebKit::WebAccessibilityCache;
using WebKit::WebAccessibilityObject;
using WebKit::WebFrame;

AccessibilityController::AccessibilityController(TestShell* shell)
    : shell_(shell) {

  BindMethod("logFocusEvents",
      &AccessibilityController::LogFocusEventsCallback);
  BindMethod("logScrollingStartEvents",
      &AccessibilityController::LogScrollingStartEventsCallback);

  BindProperty("focusedElement",
      &AccessibilityController::FocusedElementGetterCallback);
  BindProperty("rootElement",
      &AccessibilityController::RootElementGetterCallback);

  BindFallbackMethod(&AccessibilityController::FallbackCallback);
}

void AccessibilityController::BindToJavascript(
    WebFrame* frame, const std::string& classname) {
  WebAccessibilityCache::enableAccessibility();
  CppBoundClass::BindToJavascript(frame, classname);
}

void AccessibilityController::Reset() {
  root_element_ = WebAccessibilityObject();
  focused_element_ = WebAccessibilityObject();
  elements_.Clear();
}

void AccessibilityController::SetFocusedElement(
    const WebAccessibilityObject& focused_element) {
  focused_element_ = focused_element;
}

AccessibilityUIElement* AccessibilityController::GetFocusedElement() {
  if (focused_element_.isNull())
    focused_element_ = shell_->webView()->accessibilityObject();

  return elements_.Create(focused_element_);
}

AccessibilityUIElement* AccessibilityController::GetRootElement() {
  if (root_element_.isNull())
    root_element_ = shell_->webView()->accessibilityObject();
  return elements_.CreateRoot(root_element_);
}

void AccessibilityController::LogFocusEventsCallback(
    const CppArgumentList &args,
    CppVariant *result) {
  // As of r49031, this is not being used upstream.
  result->SetNull();
}

void AccessibilityController::LogScrollingStartEventsCallback(
    const CppArgumentList &args,
    CppVariant *result) {
  // As of r49031, this is not being used upstream.
  result->SetNull();
}

void AccessibilityController::FocusedElementGetterCallback(CppVariant* result) {
  result->Set(*(GetFocusedElement()->GetAsCppVariant()));
}

void AccessibilityController::RootElementGetterCallback(CppVariant *result) {
  result->Set(*(GetRootElement()->GetAsCppVariant()));
}

void AccessibilityController::FallbackCallback(const CppArgumentList &args,
                                               CppVariant *result) {
  std::wstring message(
      L"JavaScript ERROR: unknown method called on AccessibilityController");
  if (!shell_->layout_test_mode()) {
    logging::LogMessage("CONSOLE:", 0).stream() << message;
  } else {
    printf("CONSOLE MESSAGE: %S\n", message.c_str());
  }
  result->SetNull();
}
