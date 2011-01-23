// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <QtGui/QWidget>
#include <QtGui/QGroupBox>
#include <QtGui/QMainWindow>
#include <QtGui/QLayout>
#undef signals

#include "views/widget/widget_qt.h"

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/extensions/shape.h>

#include <set>
#include <vector>

#include "ui/base/dragdrop/drag_drop_types.h"
#include "base/auto_reset.h"
#include "base/compiler_specific.h"
#include "base/message_loop.h"
#include "base/utf_string_conversions.h"
#include "gfx/path.h"
#include "ui/base/dragdrop/os_exchange_data.h"
#include "ui/base/dragdrop/os_exchange_data_provider_gtk.h"
#include "views/widget/default_theme_provider.h"
#include "views/widget/drop_target_gtk.h"
#include "views/widget/gtk_views_fixed.h"
#include "views/widget/gtk_views_window.h"
#include "views/widget/root_view.h"
#include "views/widget/tooltip_manager_gtk.h"
#include "views/widget/widget_delegate.h"
#include "views/widget/widget_utils.h"
#include "views/window/window_qt.h"

using ui::OSExchangeData;
using ui::OSExchangeDataProviderGtk;
using ui::ActiveWindowWatcherX;

namespace {

}  // namespace

namespace views {

// static
//QWidget* WidgetQt::null_parent_ = NULL;

////////////////////////////////////////////////////////////////////////////////
// WidgetQt, public:

WidgetQt::WidgetQt(Type type)
    : is_window_(false),
      type_(type),
      widget_(NULL),
      focus_manager_(NULL),
      is_mouse_down_(false),
      has_capture_(false),
      last_mouse_event_was_move_(false),
      ALLOW_THIS_IN_INITIALIZER_LIST(close_widget_factory_(this)),
      delete_on_destroy_(true),
      transparent_(false),
      ignore_events_(false),
      ignore_drag_leave_(false),
      opacity_(255),
      drag_data_(NULL),
      in_paint_now_(false),
      is_active_(false),
      transient_to_parent_(false),
      got_initial_focus_in_(false),
      has_focus_(false),
      delegate_(NULL),
      always_on_top_(false),
      is_double_buffered_(false),
      should_handle_menu_key_release_(false) {
  if (type_ != TYPE_CHILD)
    focus_manager_ = new FocusManager(this);
}

WidgetQt::~WidgetQt() {
  DCHECK(delete_on_destroy_ || widget_ == NULL);
  if (type_ != TYPE_CHILD)
    ActiveWindowWatcherX::RemoveObserver(this);

  // Defer focus manager's destruction. This is for the case when the
  // focus manager is referenced by a child WidgetQt (e.g. TabbedPane in a
  // dialog). When gtk_widget_destroy is called on the parent, the destroy
  // signal reaches parent first and then the child. Thus causing the parent
  // WidgetQt's dtor executed before the child's. If child's view hierarchy
  // references this focus manager, it crashes. This will defer focus manager's
  // destruction after child WidgetQt's dtor.
  if (focus_manager_)
    MessageLoop::current()->DeleteSoon(FROM_HERE, focus_manager_);
}

bool WidgetQt::MakeTransparent() {
  // Transparency can only be enabled only if we haven't realized the widget.
  DCHECK(!widget_);

  transparent_ = true;
  return true;
}

void WidgetQt::EnableDoubleBuffer(bool enabled) {
  is_double_buffered_ = enabled;
}

bool WidgetQt::MakeIgnoreEvents() {
  // Transparency can only be enabled for windows/popups and only if we haven't
  // realized the widget.
  DCHECK(!widget_ && type_ != TYPE_CHILD);

  ignore_events_ = true;
  return true;
}

void WidgetQt::AddChild(QWidget* child) {
  layout_->addWidget(child);
}

void WidgetQt::RemoveChild(QWidget* child) {
  layout_->removeWidget(child);
}

void WidgetQt::ReparentChild(QWidget* child) {
}

void WidgetQt::PositionChild(QWidget* child, int x, int y, int w, int h) {
  child->move(x, y);
  child->resize(w, h);
}

void WidgetQt::DoDrag(const OSExchangeData& data, int operation) {
}

void WidgetQt::SetFocusTraversableParent(FocusTraversable* parent) {
  root_view_->SetFocusTraversableParent(parent);
}

void WidgetQt::SetFocusTraversableParentView(View* parent_view) {
  root_view_->SetFocusTraversableParentView(parent_view);
}

void WidgetQt::IsActiveChanged() {
  if (GetWidgetDelegate())
    GetWidgetDelegate()->IsActiveChanged(IsActive());
}

// static
WidgetQt* WidgetQt::GetViewForNative(QWidget* widget) {
    return NULL;
}

void WidgetQt::ResetDropTarget() {
  ignore_drag_leave_ = false;
 // drop_target_.reset(NULL);

}

// static
RootView* WidgetQt::GetRootViewForWidget(QWidget* widget) {
  return NULL;
}

void WidgetQt::GetRequestedSize(gfx::Size* out) const {
}

////////////////////////////////////////////////////////////////////////////////
// WidgetQt, ActiveWindowWatcherX::Observer implementation:

void WidgetQt::ActiveWindowChanged(GdkWindow* active_window) {
}

////////////////////////////////////////////////////////////////////////////////
// WidgetQt, Widget implementation:

void WidgetQt::InitWithWidget(Widget* parent,
                               const gfx::Rect& bounds) {
}

void WidgetQt::Init(QWidget* parent,
                    const gfx::Rect& bounds) {
}

WidgetDelegate* WidgetQt::GetWidgetDelegate() {
  return delegate_;
}

void WidgetQt::SetWidgetDelegate(WidgetDelegate* delegate) {
  delegate_ = delegate;
}

void WidgetQt::SetContentsView(View* view) {
  root_view_->SetContentsView(view);
}

void WidgetQt::GetBounds(gfx::Rect* out, bool including_frame) const {
}

void WidgetQt::SetBounds(const gfx::Rect& bounds) {
  if (type_ == TYPE_CHILD) {
  } else {
  }
}

void WidgetQt::MoveAbove(Widget* widget) {
}

void WidgetQt::SetShape(gfx::NativeRegion region) {
}

void WidgetQt::Close() {
  if (!widget_)
    return;  // No need to do anything.

  // Hide first.
  Hide();
  if (close_widget_factory_.empty()) {
    // And we delay the close just in case we're on the stack.
    MessageLoop::current()->PostTask(FROM_HERE,
        close_widget_factory_.NewRunnableMethod(
            &WidgetQt::CloseNow));
  }
}

void WidgetQt::CloseNow() {
  if (widget_) {
    widget_->close();
  }
}

void WidgetQt::Show() {
  if (widget_) {
    widget_->show();
  }
}

void WidgetQt::Hide() {
  if (widget_) {
    widget_->hide();
  }
}

gfx::NativeView WidgetQt::GetNativeView() const {
  return widget_;
}

void WidgetQt::PaintNow(const gfx::Rect& update_rect) {
}

void WidgetQt::SetOpacity(unsigned char opacity) {
}

void WidgetQt::SetAlwaysOnTop(bool on_top) {
}

RootView* WidgetQt::GetRootView() {
  if (!root_view_.get()) {
    // First time the root view is being asked for, create it now.
    root_view_.reset(CreateRootView());
  }
  return root_view_.get();
}

Widget* WidgetQt::GetRootWidget() const {
  return NULL;
}

bool WidgetQt::IsVisible() const {
  return widget_->isVisible();
}

bool WidgetQt::IsActive() const {
  DCHECK(type_ != TYPE_CHILD);
  return is_active_;
}

bool WidgetQt::IsAccessibleWidget() const {
  return false;
}

void WidgetQt::GenerateMousePressedForView(View* view,
                                            const gfx::Point& point) {
  NOTIMPLEMENTED();
}

TooltipManager* WidgetQt::GetTooltipManager() {
  //return tooltip_manager_.get();
  return NULL;
}

bool WidgetQt::GetAccelerator(int cmd_id, ui::Accelerator* accelerator) {
  NOTIMPLEMENTED();
  return false;
}

Window* WidgetQt::GetWindow() {
  return GetWindowImpl(widget_);
}

const Window* WidgetQt::GetWindow() const {
  return GetWindowImpl(widget_);
}

void WidgetQt::SetNativeWindowProperty(const char* name, void* value) {
  g_object_set_data(G_OBJECT(widget_), name, value);
}

void* WidgetQt::GetNativeWindowProperty(const char* name) {
  return g_object_get_data(G_OBJECT(widget_), name);
}

ThemeProvider* WidgetQt::GetThemeProvider() const {
  return GetWidgetThemeProvider(this);
}

ThemeProvider* WidgetQt::GetDefaultThemeProvider() const {
  return default_theme_provider_.get();
}

FocusManager* WidgetQt::GetFocusManager() {
  if (focus_manager_)
    return focus_manager_;

  Widget* root = GetRootWidget();
  if (root && root != this) {
    // Widget subclasses may override GetFocusManager(), for example for
    // dealing with cases where the widget has been unparented.
    return root->GetFocusManager();
  }
  return NULL;
}

void WidgetQt::ViewHierarchyChanged(bool is_add, View *parent,
                                     View *child) {
}

bool WidgetQt::ContainsNativeView(gfx::NativeView native_view) {
  // TODO(port)  See implementation in WidgetWin::ContainsNativeView.
  NOTREACHED() << "WidgetQt::ContainsNativeView is not implemented.";
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// WidgetQt, FocusTraversable implementation:

FocusSearch* WidgetQt::GetFocusSearch() {
  return root_view_->GetFocusSearch();
}

FocusTraversable* WidgetQt::GetFocusTraversableParent() {
  // We are a proxy to the root view, so we should be bypassed when traversing
  // up and as a result this should not be called.
  NOTREACHED();
  return NULL;
}

View* WidgetQt::GetFocusTraversableParentView() {
  // We are a proxy to the root view, so we should be bypassed when traversing
  // up and as a result this should not be called.
  NOTREACHED();
  return NULL;
}

void WidgetQt::ClearNativeFocus() {
  DCHECK(type_ != TYPE_CHILD);
  if (!GetNativeView()) {
    NOTREACHED();
    return;
  }
  gtk_window_set_focus(GTK_WINDOW(GetNativeView()), NULL);
}

bool WidgetQt::HandleKeyboardEvent(GdkEventKey* event) {
  return false;
}

// static
int WidgetQt::GetFlagsForEventButton(const GdkEventButton& event) {
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// WidgetQt, protected:
#if 0
void WidgetQt::OnSizeRequest(QWidget* widget, GtkRequisition* requisition) {
  // Do only return the preferred size for child windows. GtkWindow interprets
  // the requisition as a minimum size for top level windows, returning a
  // preferred size for these would prevents us from setting smaller window
  // sizes.
  if (type_ == TYPE_CHILD) {
    gfx::Size size(root_view_->GetPreferredSize());
    requisition->width = size.width();
    requisition->height = size.height();
  }
}

void WidgetQt::OnSizeAllocate(QWidget* widget, GtkAllocation* allocation) {
  // See comment next to size_ as to why we do this. Also note, it's tempting
  // to put this in the static method so subclasses don't need to worry about
  // it, but if a subclasses needs to set a shape then they need to always
  // reset the shape in this method regardless of whether the size changed.
  gfx::Size new_size(allocation->width, allocation->height);
  if (new_size == size_)
    return;
  size_ = new_size;
  root_view_->SetBounds(0, 0, allocation->width, allocation->height);
  root_view_->SchedulePaint();
}

gboolean WidgetQt::OnPaint(QWidget* widget, GdkEventExpose* event) {
  if (transparent_ && type_ == TYPE_CHILD) {
    // Clear the background before drawing any view and native components.
    DrawTransparentBackground(widget, event);
    if (!CompositePainter::IsComposited(widget_) &&
        gdk_screen_is_composited(gdk_screen_get_default())) {
      // Let the parent draw the content only after something is drawn on
      // the widget.
      CompositePainter::SetComposited(widget_);
    }
  }
  root_view_->OnPaint(event);
  return false;  // False indicates other widgets should get the event as well.
}

void WidgetQt::OnDragDataGet(QWidget* widget,
                              GdkDragContext* context,
                              GtkSelectionData* data,
                              guint info,
                              guint time) {
  if (!drag_data_) {
    NOTREACHED();
    return;
  }
  drag_data_->WriteFormatToSelection(info, data);
}

void WidgetQt::OnDragDataReceived(QWidget* widget,
                                   GdkDragContext* context,
                                   gint x,
                                   gint y,
                                   GtkSelectionData* data,
                                   guint info,
                                   guint time) {
  if (drop_target_.get())
    drop_target_->OnDragDataReceived(context, x, y, data, info, time);
}

gboolean WidgetQt::OnDragDrop(QWidget* widget,
                               GdkDragContext* context,
                               gint x,
                               gint y,
                               guint time) {
  if (drop_target_.get()) {
    return drop_target_->OnDragDrop(context, x, y, time);
  }
  return FALSE;
}

void WidgetQt::OnDragEnd(QWidget* widget, GdkDragContext* context) {
  if (!drag_data_) {
    // This indicates we didn't start a drag operation, and should never
    // happen.
    NOTREACHED();
    return;
  }
  // Quit the nested message loop we spawned in DoDrag.
  MessageLoop::current()->Quit();
}

gboolean WidgetQt::OnDragFailed(QWidget* widget,
                                 GdkDragContext* context,
                                 GtkDragResult result) {
  return FALSE;
}

void WidgetQt::OnDragLeave(QWidget* widget,
                            GdkDragContext* context,
                            guint time) {
  if (ignore_drag_leave_) {
    ignore_drag_leave_ = false;
    return;
  }
  if (drop_target_.get()) {
    drop_target_->OnDragLeave(context, time);
    drop_target_.reset(NULL);
  }
}

gboolean WidgetQt::OnDragMotion(QWidget* widget,
                                 GdkDragContext* context,
                                 gint x,
                                 gint y,
                                 guint time) {
  if (!drop_target_.get())
    drop_target_.reset(new DropTargetGtk(GetRootView(), context));
  return drop_target_->OnDragMotion(context, x, y, time);
}

gboolean WidgetQt::OnEnterNotify(QWidget* widget, GdkEventCrossing* event) {
  if (last_mouse_event_was_move_ && last_mouse_move_x_ == event->x_root &&
      last_mouse_move_y_ == event->y_root) {
    // Don't generate a mouse event for the same location as the last.
    return false;
  }

  if (has_capture_ && event->mode == GDK_CROSSING_GRAB) {
    // Doing a grab results an async enter event, regardless of where the mouse
    // is. We don't want to generate a mouse move in this case.
    return false;
  }

  if (!last_mouse_event_was_move_ && !is_mouse_down_) {
    // When a mouse button is pressed gtk generates a leave, enter, press.
    // RootView expects to get a mouse move before a press, otherwise enter is
    // not set. So we generate a move here.
    last_mouse_move_x_ = event->x_root;
    last_mouse_move_y_ = event->y_root;
    last_mouse_event_was_move_ = true;

    int x = 0, y = 0;
    GetContainedWidgetEventCoordinates(event, &x, &y);

    // If this event is the result of pressing a button then one of the button
    // modifiers is set. Unset it as we're compensating for the leave generated
    // when you press a button.
    int flags = (Event::GetFlagsFromGdkState(event->state) &
                 ~(Event::EF_LEFT_BUTTON_DOWN |
                   Event::EF_MIDDLE_BUTTON_DOWN |
                   Event::EF_RIGHT_BUTTON_DOWN));
    MouseEvent mouse_move(Event::ET_MOUSE_MOVED, x, y, flags);
    root_view_->OnMouseMoved(mouse_move);
  }

  return false;
}

gboolean WidgetQt::OnLeaveNotify(QWidget* widget, GdkEventCrossing* event) {
  last_mouse_event_was_move_ = false;
  if (!has_capture_ && !is_mouse_down_)
    root_view_->ProcessOnMouseExited();
  return false;
}

gboolean WidgetQt::OnMotionNotify(QWidget* widget, GdkEventMotion* event) {
  int x = 0, y = 0;
  GetContainedWidgetEventCoordinates(event, &x, &y);

  if (has_capture_ && is_mouse_down_) {
    last_mouse_event_was_move_ = false;
    int flags = Event::GetFlagsFromGdkState(event->state);
    MouseEvent mouse_drag(Event::ET_MOUSE_DRAGGED, x, y, flags);
    root_view_->OnMouseDragged(mouse_drag);
    return true;
  }
  gfx::Point screen_loc(event->x_root, event->y_root);
  if (last_mouse_event_was_move_ && last_mouse_move_x_ == screen_loc.x() &&
      last_mouse_move_y_ == screen_loc.y()) {
    // Don't generate a mouse event for the same location as the last.
    return true;
  }
  last_mouse_move_x_ = screen_loc.x();
  last_mouse_move_y_ = screen_loc.y();
  last_mouse_event_was_move_ = true;
  int flags = Event::GetFlagsFromGdkState(event->state);
  MouseEvent mouse_move(Event::ET_MOUSE_MOVED, x, y, flags);
  root_view_->OnMouseMoved(mouse_move);
  return true;
}

gboolean WidgetQt::OnButtonPress(QWidget* widget, GdkEventButton* event) {
  return ProcessMousePressed(event);
}

gboolean WidgetQt::OnButtonRelease(QWidget* widget, GdkEventButton* event) {
  ProcessMouseReleased(event);
  return true;
}

gboolean WidgetQt::OnScroll(QWidget* widget, GdkEventScroll* event) {
  return ProcessScroll(event);
}

gboolean WidgetQt::OnFocusIn(QWidget* widget, GdkEventFocus* event) {
  if (has_focus_)
    return false;  // This is the second focus-in event in a row, ignore it.
  has_focus_ = true;

  should_handle_menu_key_release_ = false;

  if (type_ == TYPE_CHILD)
    return false;

  // See description of got_initial_focus_in_ for details on this.
  if (!got_initial_focus_in_) {
    got_initial_focus_in_ = true;
    SetInitialFocus();
  } else {
    focus_manager_->RestoreFocusedView();
  }
  return false;
}

gboolean WidgetQt::OnFocusOut(QWidget* widget, GdkEventFocus* event) {
  if (!has_focus_)
    return false;  // This is the second focus-out event in a row, ignore it.
  has_focus_ = false;

  if (type_ == TYPE_CHILD)
    return false;

  // The top-level window lost focus, store the focused view.
  focus_manager_->StoreFocusedView();
  return false;
}

gboolean WidgetQt::OnKeyEvent(QWidget* widget, GdkEventKey* event) {
  KeyEvent key(event);

  // Always reset |should_handle_menu_key_release_| unless we are handling a
  // VKEY_MENU key release event. It ensures that VKEY_MENU accelerator can only
  // be activated when handling a VKEY_MENU key release event which is preceded
  // by an unhandled VKEY_MENU key press event. See also HandleKeyboardEvent().
  if (key.GetKeyCode() != ui::VKEY_MENU || event->type != GDK_KEY_RELEASE)
    should_handle_menu_key_release_ = false;

  bool handled = false;

  // Dispatch the key event to View hierarchy first.
  handled = root_view_->ProcessKeyEvent(key);

  // Dispatch the key event to native QWidget hierarchy.
  // To prevent GtkWindow from handling the key event as a keybinding, we need
  // to bypass GtkWindow's default key event handler and dispatch the event
  // here.
  if (!handled && GTK_IS_WINDOW(widget))
    handled = gtk_window_propagate_key_event(GTK_WINDOW(widget), event);

  // On Linux, in order to handle VKEY_MENU (Alt) accelerator key correctly and
  // avoid issues like: http://crbug.com/40966 and http://crbug.com/49701, we
  // should only send the key event to the focus manager if it's not handled by
  // any View or native QWidget.
  // The flow is different when the focus is in a RenderWidgetHostViewGtk, which
  // always consumes the key event and send it back to us later by calling
  // HandleKeyboardEvent() directly, if it's not handled by webkit.
  if (!handled)
    handled = HandleKeyboardEvent(event);

  // Dispatch the key event for bindings processing.
  if (!handled && GTK_IS_WINDOW(widget))
    handled = gtk_bindings_activate_event(GTK_OBJECT(widget), event);

  // Always return true for toplevel window to prevents GtkWindow's default key
  // event handler.
  return GTK_IS_WINDOW(widget) ? true : handled;
}

gboolean WidgetQt::OnQueryTooltip(QWidget* widget,
                                   gint x,
                                   gint y,
                                   gboolean keyboard_mode,
                                   GtkTooltip* tooltip) {
  return tooltip_manager_->ShowTooltip(x, y, keyboard_mode, tooltip);
}

gboolean WidgetQt::OnVisibilityNotify(QWidget* widget,
                                       GdkEventVisibility* event) {
  return false;
}

gboolean WidgetQt::OnGrabBrokeEvent(QWidget* widget, GdkEvent* event) {
  HandleGrabBroke();
  return false;  // To let other widgets get the event.
}

void WidgetQt::OnGrabNotify(QWidget* widget, gboolean was_grabbed) {
  if (!widget_)
    return;  // Grab broke after window destroyed, don't try processing it.

  gtk_grab_remove(widget_);
  HandleGrabBroke();
}

void WidgetQt::OnDestroy(QWidget* object) {
  // Note that this handler is hooked to GtkObject::destroy.
  // NULL out pointers here since we might still be in an observerer list
  // until delstion happens.
  widget_ = widget_ = NULL;
  delegate_ = NULL;
  if (delete_on_destroy_) {
    // Delays the deletion of this WidgetQt as we want its children to have
    // access to it when destroyed.
    MessageLoop::current()->DeleteSoon(FROM_HERE, this);
  }
}

void WidgetQt::OnShow(QWidget* widget) {
}

void WidgetQt::OnHide(QWidget* widget) {
}
#endif

void WidgetQt::DoGrab() {
  has_capture_ = true;
}

void WidgetQt::ReleaseGrab() {
  if (has_capture_) {
    has_capture_ = false;
  }
}

void WidgetQt::HandleGrabBroke() {
  if (has_capture_) {
    if (is_mouse_down_)
      root_view_->ProcessMouseDragCanceled();
    is_mouse_down_ = false;
    has_capture_ = false;
  }
}

////////////////////////////////////////////////////////////////////////////////
// WidgetQt, private:

RootView* WidgetQt::CreateRootView() {
  return new RootView(this);
}

#if 0
gboolean WidgetQt::OnWindowPaint(QWidget* widget, GdkEventExpose* event) {
  // Clear the background to be totally transparent. We don't need to
  // paint the root view here as that is done by OnPaint.
  DCHECK(transparent_);
  DrawTransparentBackground(widget, event);
  return false;
}
#endif

bool WidgetQt::ProcessMousePressed(GdkEventButton* event) {
  if (event->type == GDK_2BUTTON_PRESS || event->type == GDK_3BUTTON_PRESS) {
    // The sequence for double clicks is press, release, press, 2press, release.
    // This means that at the time we get the second 'press' we don't know
    // whether it corresponds to a double click or not. For now we're completely
    // ignoring the 2press/3press events as they are duplicate. To make this
    // work right we need to write our own code that detects if the press is a
    // double/triple. For now we're completely punting, which means we always
    // get single clicks.
    // TODO: fix this.
    return true;
  }

  // An event may come from a contained widget which has its own gdk window.
  // Translate it to the widget's coordinates.
  int x = 0;
  int y = 0;
  GetContainedWidgetEventCoordinates(event, &x, &y);
  last_mouse_event_was_move_ = false;
  MouseEvent mouse_pressed(Event::ET_MOUSE_PRESSED, x, y,
                           GetFlagsForEventButton(*event));

  if (root_view_->OnMousePressed(mouse_pressed)) {
    is_mouse_down_ = true;
    if (!has_capture_)
      DoGrab();
    return true;
  }

  // Returns true to consume the event when widget is not transparent.
  return !transparent_;
}

void WidgetQt::ProcessMouseReleased(GdkEventButton* event) {
  int x = 0, y = 0;
  GetContainedWidgetEventCoordinates(event, &x, &y);

  last_mouse_event_was_move_ = false;
  MouseEvent mouse_up(Event::ET_MOUSE_RELEASED, x, y,
                      GetFlagsForEventButton(*event));
  // Release the capture first, that way we don't get confused if
  // OnMouseReleased blocks.
  if (has_capture_ && ReleaseCaptureOnMouseReleased())
    ReleaseGrab();
  is_mouse_down_ = false;
  // GTK generates a mouse release at the end of dnd. We need to ignore it.
  if (!drag_data_)
    root_view_->OnMouseReleased(mouse_up, false);
}

bool WidgetQt::ProcessScroll(GdkEventScroll* event) {
  return false;
}

// static
void WidgetQt::SetRootViewForWidget(QWidget* widget, RootView* root_view) {
}

// static
Window* WidgetQt::GetWindowImpl(QWidget* widget) {
  return NULL;
}

void WidgetQt::CreateQtWidget(QWidget* parent, const gfx::Rect& bounds) {
  if (type_ == TYPE_CHILD) {
    widget_ = new QGroupBox();
  } else {
    widget_ = new QMainWindow();
  }
//  layout_ = new QLayout(widget_);
}

void WidgetQt::ConfigureWidgetForTransparentBackground(QWidget* parent) {
  DCHECK(widget_ && widget_);
}

void WidgetQt::ConfigureWidgetForIgnoreEvents() {
}

void WidgetQt::DrawTransparentBackground(QWidget* widget,
                                          GdkEventExpose* event) {
}

////////////////////////////////////////////////////////////////////////////////
// Widget, public:

// static
Widget* Widget::CreatePopupWidget(TransparencyParam transparent,
                                  EventsParam accept_events,
                                  DeleteParam delete_on_destroy,
                                  MirroringParam mirror_in_rtl) {
  WidgetQt* popup = new WidgetQt(WidgetQt::TYPE_POPUP);
  popup->set_delete_on_destroy(delete_on_destroy == DeleteOnDestroy);
  if (transparent == Transparent)
    popup->MakeTransparent();
  if (accept_events == NotAcceptEvents)
    popup->MakeIgnoreEvents();
  return popup;
}

// static
RootView* Widget::FindRootView(QMainWindow* window) {
  return NULL;
}

static void AllRootViewsLocatorCallback(QWidget* widget,
                                        gpointer root_view_p) {
}

// static
void Widget::FindAllRootViews(QMainWindow* window,
                              std::vector<RootView*>* root_views) {
}

// static
Widget* Widget::GetWidgetFromNativeView(gfx::NativeView native_view) {
  return NULL;
}

// static
Widget* Widget::GetWidgetFromNativeWindow(gfx::NativeWindow native_window) {
  return NULL;
}

// static
void Widget::NotifyLocaleChanged() {
}

}  // namespace views

