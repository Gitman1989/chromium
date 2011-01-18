// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


/*
  EventSendingController class:
  Bound to a JavaScript window.eventSender object using
  CppBoundClass::BindToJavascript(), this allows layout tests that are run in
  the test_shell to fire DOM events.

  The OSX reference file is in
  WebKit/Tools/DumpRenderTree/EventSendingController.m
*/

#ifndef WEBKIT_TOOLS_TEST_SHELL_EVENT_SENDING_CONTROLLER_H_
#define WEBKIT_TOOLS_TEST_SHELL_EVENT_SENDING_CONTROLLER_H_

#include "build/build_config.h"
#include "gfx/point.h"
#include "base/task.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebDragOperation.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebInputEvent.h"
#include "webkit/glue/cpp_bound_class.h"

class TestShell;

namespace WebKit {
class WebDragData;
class WebView;
struct WebPoint;
}

class EventSendingController : public CppBoundClass {
 public:
  // Builds the property and method lists needed to bind this class to a JS
  // object.
  EventSendingController(TestShell* shell);
  ~EventSendingController();

  // Resets some static variable state.
  void Reset();

  // Simulate drag&drop system call.
  void DoDragDrop(const WebKit::WebDragData& drag_data,
                  WebKit::WebDragOperationsMask operations_mask);

  // JS callback methods.
  void mouseDown(const CppArgumentList& args, CppVariant* result);
  void mouseUp(const CppArgumentList& args, CppVariant* result);
  void mouseMoveTo(const CppArgumentList& args, CppVariant* result);
  void leapForward(const CppArgumentList& args, CppVariant* result);
  void keyDown(const CppArgumentList& args, CppVariant* result);
  void dispatchMessage(const CppArgumentList& args, CppVariant* result);
  void textZoomIn(const CppArgumentList& args, CppVariant* result);
  void textZoomOut(const CppArgumentList& args, CppVariant* result);
  void zoomPageIn(const CppArgumentList& args, CppVariant* result);
  void zoomPageOut(const CppArgumentList& args, CppVariant* result);
  void mouseScrollBy(const CppArgumentList& args, CppVariant* result);
  void continuousMouseScrollBy(const CppArgumentList& args, CppVariant* result);
  void scheduleAsynchronousClick(const CppArgumentList& args,
                                 CppVariant* result);
  void beginDragWithFiles(const CppArgumentList& args, CppVariant* result);
  CppVariant dragMode;

  void addTouchPoint(const CppArgumentList& args, CppVariant* result);
  void cancelTouchPoint(const CppArgumentList& args, CppVariant* result);
  void clearTouchPoints(const CppArgumentList& args, CppVariant* result);
  void releaseTouchPoint(const CppArgumentList& args, CppVariant* result);
  void setTouchModifier(const CppArgumentList& args, CppVariant* result);
  void touchCancel(const CppArgumentList& args, CppVariant* result);
  void touchEnd(const CppArgumentList& args, CppVariant* result);
  void touchMove(const CppArgumentList& args, CppVariant* result);
  void touchStart(const CppArgumentList& args, CppVariant* result);
  void updateTouchPoint(const CppArgumentList& args, CppVariant* result);

  // Unimplemented stubs
  void contextClick(const CppArgumentList& args, CppVariant* result);
  void enableDOMUIEventLogging(const CppArgumentList& args, CppVariant* result);
  void fireKeyboardEventsToElement(const CppArgumentList& args,
                                   CppVariant* result);
  void clearKillRing(const CppArgumentList& args, CppVariant* result);

  // Properties used in layout tests.
#if defined(OS_WIN)
  CppVariant wmKeyDown;
  CppVariant wmKeyUp;
  CppVariant wmChar;
  CppVariant wmDeadChar;
  CppVariant wmSysKeyDown;
  CppVariant wmSysKeyUp;
  CppVariant wmSysChar;
  CppVariant wmSysDeadChar;
#endif

 private:
  // Returns the test shell's webview.
  WebKit::WebView* webview();

  // Returns true if dragMode is true.
  bool drag_mode() { return dragMode.isBool() && dragMode.ToBoolean(); }

  // Sometimes we queue up mouse move and mouse up events for drag drop
  // handling purposes.  These methods dispatch the event.
  void DoMouseMove(const WebKit::WebMouseEvent& e);
  void DoMouseUp(const WebKit::WebMouseEvent& e);
  static void DoLeapForward(int milliseconds);
  void ReplaySavedEvents();

  // Helper to return the button type given a button code
  static WebKit::WebMouseEvent::Button GetButtonTypeFromButtonNumber(
      int button_code);

  // Helper to extract the button number from the optional argument in
  // mouseDown and mouseUp
  static int GetButtonNumberFromSingleArg(const CppArgumentList& args);

  // Returns true if the key_code passed in needs a shift key modifier to
  // be passed into the generated event.
  bool NeedsShiftModifier(int key_code);

  void UpdateClickCountForButton(WebKit::WebMouseEvent::Button button_type);

  // Compose a touch event from the current touch points and send it.
  void SendCurrentTouchEvent(const WebKit::WebInputEvent::Type type);

  // Handle a request to send a wheel event.
  void handleMouseWheel(const CppArgumentList&, CppVariant*, bool continuous);

  ScopedRunnableMethodFactory<EventSendingController> method_factory_;

  // Non-owning pointer.  The LayoutTestController is owned by the host.
  TestShell* shell_;

  // Location of last mouseMoveTo event.
  static gfx::Point last_mouse_pos_;

  // Currently pressed mouse button (Left/Right/Middle or None)
  static WebKit::WebMouseEvent::Button pressed_button_;

  // The last button number passed to mouseDown and mouseUp.
  // Used to determine whether the click count continues to
  // increment or not.
  static WebKit::WebMouseEvent::Button last_button_type_;
};

#endif  // WEBKIT_TOOLS_TEST_SHELL_EVENT_SENDING_CONTROLLER_H_
