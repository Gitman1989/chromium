// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VIEWS_WIDGET_WIDGET_GTK_H_
#define VIEWS_WIDGET_WIDGET_GTK_H_
#pragma once

#include <gtk/gtk.h>

#include "app/active_window_watcher_x.h"
#include "app/gtk_signal.h"
#include "base/message_loop.h"
#include "gfx/size.h"
#include "views/focus/focus_manager.h"
#include "views/widget/widget.h"

namespace gfx {
class Rect;
}

namespace ui {
class OSExchangeData;
class OSExchangeDataProviderGtk;
}
using ui::OSExchangeData;
using ui::OSExchangeDataProviderGtk;

namespace views {

class DefaultThemeProvider;
class DropTargetGtk;
class FocusSearch;
class TooltipManagerGtk;
class View;
class WindowGtk;

// Widget implementation for GTK.
class WidgetGtk
    : public Widget,
      public FocusTraversable,
      public ActiveWindowWatcherX::Observer {
 public:
  // Type of widget.
  enum Type {
    // Used for popup type windows (bubbles, menus ...).
    // NOTE: on X windows of this type can NOT get focus. If you need a popup
    // like widget that can be focused use TYPE_WINDOW and set the window type
    // to WINDOW_TYPE_CHROME_INFO_BUBBLE.
    TYPE_POPUP,

    // A top level window with no title or control buttons.
    TYPE_WINDOW,

    // A top level, decorated window.
    TYPE_DECORATED_WINDOW,

    // A child widget.
    TYPE_CHILD
  };

  explicit WidgetGtk(Type type);
  virtual ~WidgetGtk();

  // Marks this window as transient to its parent. A window that is transient
  // to its parent results in the parent rendering active when the child is
  // active.
  // This must be invoked before Init. This is only used for types other than
  // TYPE_CHILD. The default is false.
  // See gtk_window_set_transient_for for details.
  void make_transient_to_parent() {
    DCHECK(!widget_);
    transient_to_parent_ = true;
  }

  // Returns the transient parent. See make_transient_to_parent for details on
  // what the transient parent is.
  GtkWindow* GetTransientParent() const;

  // Makes the background of the window totally transparent. This must be
  // invoked before Init. This does a couple of checks and returns true if
  // the window can be made transparent. The actual work of making the window
  // transparent is done by ConfigureWidgetForTransparentBackground.
  // This works for both child and window types.
  bool MakeTransparent();
  bool is_transparent() const { return transparent_; }

  // Enable/Disable double buffering.This is neccessary to prevent
  // flickering in ScrollView, which has both native and view
  // controls.
  void EnableDoubleBuffer(bool enabled);
  bool is_double_buffered() const { return is_double_buffered_; }

  // Makes the window pass all events through to any windows behind it.
  // This must be invoked before Init. This does a couple of checks and returns
  // true if the window can be made to ignore events. The actual work of making
  // the window ignore events is done by ConfigureWidgetForIgnoreEvents.
  bool MakeIgnoreEvents();
  bool is_ignore_events() const { return ignore_events_; }

  // Sets whether or not we are deleted when the widget is destroyed. The
  // default is true.
  void set_delete_on_destroy(bool delete_on_destroy) {
    delete_on_destroy_ = delete_on_destroy;
  }

  // Adds and removes the specified widget as a child of this widget's contents.
  // These methods make sure to add the widget to the window's contents
  // container if this widget is a window.
  void AddChild(GtkWidget* child);
  void RemoveChild(GtkWidget* child);

  // A safe way to reparent a child widget to this widget. Calls
  // gtk_widget_reparent which handles refcounting to avoid destroying the
  // widget when removing it from its old parent.
  void ReparentChild(GtkWidget* child);

  // Positions a child GtkWidget at the specified location and bounds.
  void PositionChild(GtkWidget* child, int x, int y, int w, int h);

  // Parent GtkWidget all children are added to. When this WidgetGtk corresponds
  // to a top level window, this is the GtkFixed within the GtkWindow, not the
  // GtkWindow itself. For child widgets, this is the same GtkFixed as
  // |widget_|.
  GtkWidget* window_contents() const { return window_contents_; }

  // Starts a drag on this widget. This blocks until the drag is done.
  void DoDrag(const OSExchangeData& data, int operation);

  // Are we in PaintNow? See use in root_view_gtk for details on what this is
  // used for.
  bool in_paint_now() const { return in_paint_now_; }

  // Sets the focus traversable parents.
  void SetFocusTraversableParent(FocusTraversable* parent);
  void SetFocusTraversableParentView(View* parent_view);

  // Invoked when the active status changes.
  virtual void IsActiveChanged();

  // Sets initial focus on a new window. On X11/Gtk, window creation
  // is asynchronous and a focus request has to be made after a window
  // gets created. This will not be called on a TYPE_CHILD widget.
  virtual void SetInitialFocus() {}

  // Gets the WidgetGtk in the userdata section of the widget.
  static WidgetGtk* GetViewForNative(GtkWidget* widget);

  // Sets the drop target to NULL. This is invoked by DropTargetGTK when the
  // drop is done.
  void ResetDropTarget();

  // Returns the RootView for |widget|.
  static RootView* GetRootViewForWidget(GtkWidget* widget);

  // Gets the requested size of the widget.  This can be the size
  // stored in properties for a GtkViewsFixed, or in the requisitioned
  // size of other kinds of widgets.
  void GetRequestedSize(gfx::Size* out) const;

  // Overriden from ActiveWindowWatcherX::Observer.
  virtual void ActiveWindowChanged(GdkWindow* active_window);

  // Overridden from Widget:
  virtual void Init(gfx::NativeView parent, const gfx::Rect& bounds);
  virtual void InitWithWidget(Widget* parent, const gfx::Rect& bounds);
  virtual WidgetDelegate* GetWidgetDelegate();
  virtual void SetWidgetDelegate(WidgetDelegate* delegate);
  virtual void SetContentsView(View* view);
  virtual void GetBounds(gfx::Rect* out, bool including_frame) const;
  virtual void SetBounds(const gfx::Rect& bounds);
  virtual void MoveAbove(Widget* other);
  virtual void SetShape(gfx::NativeRegion region);
  virtual void Close();
  virtual void CloseNow();
  virtual void Show();
  virtual void Hide();
  virtual gfx::NativeView GetNativeView() const;
  virtual void PaintNow(const gfx::Rect& update_rect);
  virtual void SetOpacity(unsigned char opacity);
  virtual void SetAlwaysOnTop(bool on_top);
  virtual RootView* GetRootView();
  virtual Widget* GetRootWidget() const;
  virtual bool IsVisible() const;
  virtual bool IsActive() const;
  virtual bool IsAccessibleWidget() const;
  virtual void GenerateMousePressedForView(View* view,
                                           const gfx::Point& point);
  virtual TooltipManager* GetTooltipManager();
  virtual bool GetAccelerator(int cmd_id, ui::Accelerator* accelerator);
  virtual Window* GetWindow();
  virtual const Window* GetWindow() const;
  virtual void SetNativeWindowProperty(const char* name, void* value);
  virtual void* GetNativeWindowProperty(const char* name);
  virtual ThemeProvider* GetThemeProvider() const;
  virtual ThemeProvider* GetDefaultThemeProvider() const;
  virtual FocusManager* GetFocusManager();
  virtual void ViewHierarchyChanged(bool is_add, View *parent,
                                    View *child);
  virtual bool ContainsNativeView(gfx::NativeView native_view);

  // Overridden from FocusTraversable:
  virtual FocusSearch* GetFocusSearch();
  virtual FocusTraversable* GetFocusTraversableParent();
  virtual View* GetFocusTraversableParentView();

  // Clears the focus on the native widget having the focus.
  virtual void ClearNativeFocus();

  // Handles a keyboard event by sending it to our focus manager.
  // Returns true if it's handled by the focus manager.
  bool HandleKeyboardEvent(GdkEventKey* event);

  // Returns the view::Event::flags for a GdkEventButton.
  static int GetFlagsForEventButton(const GdkEventButton& event);

 protected:
  // If widget containes another widget, translates event coordinates to the
  // contained widget's coordinates, else returns original event coordinates.
  template<class Event> bool GetContainedWidgetEventCoordinates(Event* event,
                                                                int* x,
                                                                int* y) {
    if (event == NULL || x == NULL || y == NULL)
      return false;
    *x = event->x;
    *y = event->y;
    GdkWindow* dest = GTK_WIDGET(window_contents_)->window;
    if (event->window != dest) {
      int dest_x, dest_y;
      gdk_window_get_root_origin(dest, &dest_x, &dest_y);
      *x = event->x_root - dest_x;
      *y = event->y_root - dest_y;
      return true;
    }
    return false;
  }

  // Event handlers:
  CHROMEGTK_CALLBACK_1(WidgetGtk, gboolean, OnButtonPress, GdkEventButton*);
  CHROMEGTK_CALLBACK_1(WidgetGtk, void, OnSizeRequest, GtkRequisition*);
  CHROMEGTK_CALLBACK_1(WidgetGtk, void, OnSizeAllocate, GtkAllocation*);
  CHROMEGTK_CALLBACK_1(WidgetGtk, gboolean, OnPaint, GdkEventExpose*);
  CHROMEGTK_CALLBACK_4(WidgetGtk, void, OnDragDataGet,
                       GdkDragContext*, GtkSelectionData*, guint, guint);
  CHROMEGTK_CALLBACK_6(WidgetGtk, void, OnDragDataReceived,
                       GdkDragContext*, gint, gint, GtkSelectionData*,
                       guint, guint);
  CHROMEGTK_CALLBACK_4(WidgetGtk, gboolean, OnDragDrop,
                       GdkDragContext*, gint, gint, guint);
  CHROMEGTK_CALLBACK_1(WidgetGtk, void, OnDragEnd, GdkDragContext*);
  CHROMEGTK_CALLBACK_2(WidgetGtk, gboolean, OnDragFailed,
                       GdkDragContext*, GtkDragResult);
  CHROMEGTK_CALLBACK_2(WidgetGtk, void, OnDragLeave,
                       GdkDragContext*, guint);
  CHROMEGTK_CALLBACK_4(WidgetGtk, gboolean, OnDragMotion,
                       GdkDragContext*, gint, gint, guint);
  CHROMEGTK_CALLBACK_1(WidgetGtk, gboolean, OnEnterNotify, GdkEventCrossing*);
  CHROMEGTK_CALLBACK_1(WidgetGtk, gboolean, OnLeaveNotify, GdkEventCrossing*);
  CHROMEGTK_CALLBACK_1(WidgetGtk, gboolean, OnMotionNotify, GdkEventMotion*);
  CHROMEGTK_CALLBACK_1(WidgetGtk, gboolean, OnButtonRelease, GdkEventButton*);
  CHROMEGTK_CALLBACK_1(WidgetGtk, gboolean, OnFocusIn, GdkEventFocus*);
  CHROMEGTK_CALLBACK_1(WidgetGtk, gboolean, OnFocusOut, GdkEventFocus*);
  CHROMEGTK_CALLBACK_1(WidgetGtk, gboolean, OnKeyEvent, GdkEventKey*);
  CHROMEGTK_CALLBACK_4(WidgetGtk, gboolean, OnQueryTooltip,
                       gint, gint, gboolean, GtkTooltip*);
  CHROMEGTK_CALLBACK_1(WidgetGtk, gboolean, OnScroll, GdkEventScroll*);
  CHROMEGTK_CALLBACK_1(WidgetGtk, gboolean, OnVisibilityNotify,
                       GdkEventVisibility*);
  CHROMEGTK_CALLBACK_1(WidgetGtk, gboolean, OnGrabBrokeEvent, GdkEvent*);
  CHROMEGTK_CALLBACK_1(WidgetGtk, void, OnGrabNotify, gboolean);
  CHROMEGTK_CALLBACK_0(WidgetGtk, void, OnDestroy);
  CHROMEGTK_CALLBACK_0(WidgetGtk, void, OnShow);
  CHROMEGTK_CALLBACK_0(WidgetGtk, void, OnHide);

  void set_mouse_down(bool mouse_down) { is_mouse_down_ = mouse_down; }

  // Do we own the mouse grab?
  bool has_capture() const { return has_capture_; }

  // Returns whether capture should be released on mouse release. The default
  // is true.
  virtual bool ReleaseCaptureOnMouseReleased() { return true; }

  // Does a mouse grab on this widget.
  virtual void DoGrab();

  // Releases a grab done by this widget.
  virtual void ReleaseGrab();

  // Invoked when input grab is stolen by other GtkWidget in the same
  // application.
  virtual void HandleGrabBroke();

  // Are we a subclass of WindowGtk?
  bool is_window_;

  // For test code to provide a customized focus manager.
  void set_focus_manager(FocusManager* focus_manager) {
    delete focus_manager_;
    focus_manager_ = focus_manager;
  }

 private:
  class DropObserver;
  friend class DropObserver;

  virtual RootView* CreateRootView();

  CHROMEGTK_CALLBACK_1(WidgetGtk, gboolean, OnWindowPaint, GdkEventExpose*);

  // Process a mouse click.
  bool ProcessMousePressed(GdkEventButton* event);
  void ProcessMouseReleased(GdkEventButton* event);
  // Process scroll event.
  bool ProcessScroll(GdkEventScroll* event);

  static void SetRootViewForWidget(GtkWidget* widget, RootView* root_view);

  // Returns the first ancestor of |widget| that is a window.
  static Window* GetWindowImpl(GtkWidget* widget);

  // Creates the GtkWidget.
  void CreateGtkWidget(GtkWidget* parent, const gfx::Rect& bounds);

  // Invoked from create widget to enable the various bits needed for a
  // transparent background. This is only invoked if MakeTransparent has been
  // invoked.
  void ConfigureWidgetForTransparentBackground(GtkWidget* parent);

  // Invoked from create widget to enable the various bits needed for a
  // window which doesn't receive events. This is only invoked if
  // MakeIgnoreEvents has been invoked.
  void ConfigureWidgetForIgnoreEvents();

  // A utility function to draw a transparent background onto the |widget|.
  static void DrawTransparentBackground(GtkWidget* widget,
                                        GdkEventExpose* event);

  const Type type_;

  // Our native views. If we're a window/popup, then widget_ is the window and
  // window_contents_ is a GtkFixed. If we're not a window/popup, then widget_
  // and window_contents_ point to the same GtkFixed.
  GtkWidget* widget_;
  GtkWidget* window_contents_;

  // Child GtkWidgets created with no parent need to be parented to a valid top
  // level window otherwise Gtk throws a fit. |null_parent_| is an invisible
  // popup that such GtkWidgets are parented to.
  static GtkWidget* null_parent_;

  // The TooltipManager.
  // WARNING: RootView's destructor calls into the TooltipManager. As such, this
  // must be destroyed AFTER root_view_.
  scoped_ptr<TooltipManagerGtk> tooltip_manager_;

  scoped_ptr<DropTargetGtk> drop_target_;

  // The focus manager keeping track of focus for this Widget and any of its
  // children.  NULL for non top-level widgets.
  // WARNING: RootView's destructor calls into the FocusManager. As such, this
  // must be destroyed AFTER root_view_.
  FocusManager* focus_manager_;

  // The root of the View hierarchy attached to this window.
  scoped_ptr<RootView> root_view_;

  // If true, the mouse is currently down.
  bool is_mouse_down_;

  // Have we done a mouse grab?
  bool has_capture_;

  // The following are used to detect duplicate mouse move events and not
  // deliver them. Displaying a window may result in the system generating
  // duplicate move events even though the mouse hasn't moved.

  // If true, the last event was a mouse move event.
  bool last_mouse_event_was_move_;

  // Coordinates of the last mouse move event, in screen coordinates.
  int last_mouse_move_x_;
  int last_mouse_move_y_;

  // The following factory is used to delay destruction.
  ScopedRunnableMethodFactory<WidgetGtk> close_widget_factory_;

  // See description above setter.
  bool delete_on_destroy_;

  // See description above make_transparent for details.
  bool transparent_;

  // See description above MakeIgnoreEvents for details.
  bool ignore_events_;

  scoped_ptr<DefaultThemeProvider> default_theme_provider_;

  // See note in DropObserver for details on this.
  bool ignore_drag_leave_;

  unsigned char opacity_;

  // This is non-null during the life of DoDrag and contains the actual data
  // for the drag.
  const OSExchangeDataProviderGtk* drag_data_;

  // See description above getter for details.
  bool in_paint_now_;

  // Are we active?
  bool is_active_;

  // See make_transient_to_parent for a description.
  bool transient_to_parent_;

  // Last size supplied to OnSizeAllocate. We cache this as any time the
  // size of a GtkWidget changes size_allocate is called, even if the size
  // didn't change. If we didn't cache this and ignore calls when the size
  // hasn't changed, we can end up getting stuck in a never ending loop.
  gfx::Size size_;

  // This is initially false and when the first focus-in event is received this
  // is set to true and no additional processing is done. Subsequently when
  // focus-in is received we do the normal focus manager processing.
  //
  // This behavior is necessitated by Gtk/X sending focus events
  // asynchronously. The initial sequence for windows is typically: show,
  // request focus on some widget. Because of async events on Gtk this becomes
  // show, request focus, get focus in event which ends up clearing focus
  // (first request to FocusManager::RestoreFocusedView ends up clearing focus).
  bool got_initial_focus_in_;

  // If true, we've received a focus-in event. If false we've received a
  // focus-out event. We can get multiple focus-out events in a row, we use
  // this to determine whether we should process the event.
  bool has_focus_;

  // Non owned pointer to optional delegate.  May be NULL if no delegate is
  // being used.
  WidgetDelegate* delegate_;

  // If true, the window stays on top of the screen. This is only used
  // for types other than TYPE_CHILD.
  bool always_on_top_;

  // If true, we enable the content widget's double buffering.
  // This is false by default.
  bool is_double_buffered_;

  // Indicates if we should handle the upcoming Alt key release event.
  bool should_handle_menu_key_release_;

  DISALLOW_COPY_AND_ASSIGN(WidgetGtk);
};

}  // namespace views

#endif  // VIEWS_WIDGET_WIDGET_GTK_H_
