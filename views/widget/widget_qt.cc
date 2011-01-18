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

#include "app/drag_drop_types.h"
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
#include "views/window/window_gtk.h"

using ui::OSExchangeData;
using ui::OSExchangeDataProviderGtk;

namespace {

// g_object data keys to associate a WidgetQt object to a QWidget.
const char* kWidgetKey = "__VIEWS_WIDGET__";
// A g_object data key to associate a CompositePainter object to a QWidget.
const char* kCompositePainterKey = "__VIEWS_COMPOSITE_PAINTER__";
// A g_object data key to associate the flag whether or not the widget
// is composited to a QWidget. gtk_widget_is_composited simply tells
// if x11 supports composition and cannot be used to tell if given widget
// is composited.
const char* kCompositeEnabledKey = "__VIEWS_COMPOSITE_ENABLED__";

// CompositePainter draws a composited child widgets image into its
// drawing area. This object is created at most once for a widget and kept
// until the widget is destroyed.
class CompositePainter {
 public:
  explicit CompositePainter(QWidget* parent)
      : parent_object_(G_OBJECT(parent)) {
    handler_id_ = g_signal_connect_after(
        parent_object_, "expose_event", G_CALLBACK(OnCompositePaint), NULL);
  }

  static void AddCompositePainter(QWidget* widget) {
    CompositePainter* painter = static_cast<CompositePainter*>(
        g_object_get_data(G_OBJECT(widget), kCompositePainterKey));
    if (!painter) {
      g_object_set_data(G_OBJECT(widget), kCompositePainterKey,
                        new CompositePainter(widget));
      g_signal_connect(widget, "destroy",
                       G_CALLBACK(&DestroyPainter), NULL);
    }
  }

  // Set the composition flag.
  static void SetComposited(QWidget* widget) {
    g_object_set_data(G_OBJECT(widget), kCompositeEnabledKey,
                      const_cast<char*>(""));
  }

  // Returns true if the |widget| is composited and ready to be drawn.
  static bool IsComposited(QWidget* widget) {
    return g_object_get_data(G_OBJECT(widget), kCompositeEnabledKey) != NULL;
  }

 private:
  virtual ~CompositePainter() {}

  // Composes a image from one child.
  static void CompositeChildWidget(QWidget* child, gpointer data) {
    GdkEventExpose* event = static_cast<GdkEventExpose*>(data);
    QWidget* parent = gtk_widget_get_parent(child);
    DCHECK(parent);
    if (IsComposited(child)) {
      cairo_t* cr = gdk_cairo_create(parent->window);
      gdk_cairo_set_source_pixmap(cr, child->window,
                                  child->allocation.x,
                                  child->allocation.y);
      GdkRegion* region = gdk_region_rectangle(&child->allocation);
      gdk_region_intersect(region, event->region);
      gdk_cairo_region(cr, region);
      cairo_clip(cr);
      cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
      cairo_paint(cr);
      cairo_destroy(cr);
    }
  }

  // Expose-event handler that compose & draws children's image into
  // the |parent|'s drawing area.
  static gboolean OnCompositePaint(QWidget* parent, GdkEventExpose* event) {
    gtk_container_foreach(GTK_CONTAINER(parent),
                          CompositeChildWidget,
                          event);
    return false;
  }

  static void DestroyPainter(QWidget* object) {
    CompositePainter* painter = reinterpret_cast<CompositePainter*>(
        g_object_get_data(G_OBJECT(object), kCompositePainterKey));
    DCHECK(painter);
    delete painter;
  }

  GObject* parent_object_;
  gulong handler_id_;

  DISALLOW_COPY_AND_ASSIGN(CompositePainter);
};

}  // namespace

namespace views {

// During drag and drop GTK sends a drag-leave during a drop. This means we
// have no way to tell the difference between a normal drag leave and a drop.
// To work around that we listen for DROP_START, then ignore the subsequent
// drag-leave that GTK generates.
class WidgetQt::DropObserver : public MessageLoopForUI::Observer {
 public:
  DropObserver() {}

  static DropObserver* GetInstance() {
    return Singleton<DropObserver>::get();
  }

  virtual void WillProcessEvent(GdkEvent* event) {
    if (event->type == GDK_DROP_START) {
      WidgetQt* widget = GetWidgetGtkForEvent(event);
      if (widget)
        widget->ignore_drag_leave_ = true;
    }
  }

  virtual void DidProcessEvent(GdkEvent* event) {
  }

 private:
  WidgetQt* GetWidgetGtkForEvent(GdkEvent* event) {
    QWidget* gtk_widget = gtk_get_event_widget(event);
    if (!gtk_widget)
      return NULL;

    return WidgetQt::GetViewForNative(gtk_widget);
  }

  DISALLOW_COPY_AND_ASSIGN(DropObserver);
};

// Returns the position of a widget on screen.
static void GetWidgetPositionOnScreen(QWidget* widget, int* x, int *y) {
  // First get the root window.
  QWidget* root = widget;
  while (root && !GTK_IS_WINDOW(root)) {
    root = gtk_widget_get_parent(root);
  }
  if (!root) {
    // If root is null we're not parented. Return 0x0 and assume the caller will
    // query again when we're parented.
    *x = *y = 0;
    return;
  }
  // Translate the coordinate from widget to root window.
  gtk_widget_translate_coordinates(widget, root, 0, 0, x, y);
  // Then adjust the position with the position of the root window.
  int window_x, window_y;
  gtk_window_get_position(GTK_WINDOW(root), &window_x, &window_y);
  *x += window_x;
  *y += window_y;
}

// "expose-event" handler of drag icon widget that renders drag image pixbuf.
static gboolean DragIconWidgetPaint(QWidget* widget,
                                    GdkEventExpose* event,
                                    gpointer data) {
  GdkPixbuf* pixbuf = reinterpret_cast<GdkPixbuf*>(data);

  cairo_t* cr = gdk_cairo_create(widget->window);

  gdk_cairo_region(cr, event->region);
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  gdk_cairo_set_source_pixbuf(cr, pixbuf, 0.0, 0.0);
  cairo_paint(cr);

  cairo_destroy(cr);
  return true;
}

// Creates a drag icon widget that draws drag_image.
static QWidget* CreateDragIconWidget(GdkPixbuf* drag_image) {
  GdkColormap* rgba_colormap =
      gdk_screen_get_rgba_colormap(gdk_screen_get_default());
  if (!rgba_colormap)
    return NULL;

  QWidget* drag_widget = gtk_window_new(GTK_WINDOW_POPUP);

  gtk_widget_set_colormap(drag_widget, rgba_colormap);
  gtk_widget_set_app_paintable(drag_widget, true);
  gtk_widget_set_size_request(drag_widget,
                              gdk_pixbuf_get_width(drag_image),
                              gdk_pixbuf_get_height(drag_image));

  g_signal_connect(G_OBJECT(drag_widget), "expose-event",
                   G_CALLBACK(&DragIconWidgetPaint), drag_image);
  return drag_widget;
}

// static
QWidget* WidgetQt::null_parent_ = NULL;

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
  static bool installed_message_loop_observer = false;
  if (!installed_message_loop_observer) {
    installed_message_loop_observer = true;
    MessageLoopForUI* loop = MessageLoopForUI::current();
    if (loop)
      loop->AddObserver(DropObserver::GetInstance());
  }

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

GtkWindow* WidgetQt::GetTransientParent() const {
  return (type_ != TYPE_CHILD && widget_) ?
      gtk_window_get_transient_for(GTK_WINDOW(widget_)) : NULL;
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
  return static_cast<WidgetQt*>(GetWidgetFromNativeView(widget));
}

void WidgetQt::ResetDropTarget() {
  ignore_drag_leave_ = false;
  drop_target_.reset(NULL);
}

// static
RootView* WidgetQt::GetRootViewForWidget(QWidget* widget) {
  gpointer user_data = g_object_get_data(G_OBJECT(widget), "root-view");
  return static_cast<RootView*>(user_data);
}

void WidgetQt::GetRequestedSize(gfx::Size* out) const {
  int width, height;
  if (GTK_IS_VIEWS_FIXED(widget_) &&
      gtk_views_fixed_get_widget_size(GetNativeView(), &width, &height)) {
    out->SetSize(width, height);
  } else {
    GtkRequisition requisition;
    gtk_widget_get_child_requisition(GetNativeView(), &requisition);
    out->SetSize(requisition.width, requisition.height);
  }
}

////////////////////////////////////////////////////////////////////////////////
// WidgetQt, ActiveWindowWatcherX::Observer implementation:

void WidgetQt::ActiveWindowChanged(GdkWindow* active_window) {
  if (!GetNativeView())
    return;

  bool was_active = IsActive();
  is_active_ = (active_window == GTK_WIDGET(GetNativeView())->window);
  if (!is_active_ && active_window && type_ != TYPE_CHILD) {
    // We're not active, but the force the window to be rendered as active if
    // a child window is transient to us.
    gpointer data = NULL;
    gdk_window_get_user_data(active_window, &data);
    QWidget* widget = reinterpret_cast<QWidget*>(data);
    is_active_ =
        (widget && GTK_IS_WINDOW(widget) &&
         gtk_window_get_transient_for(GTK_WINDOW(widget)) == GTK_WINDOW(
             widget_));
  }
  if (was_active != IsActive())
    IsActiveChanged();
}

////////////////////////////////////////////////////////////////////////////////
// WidgetQt, Widget implementation:

void WidgetQt::InitWithWidget(Widget* parent,
                               const gfx::Rect& bounds) {
  WidgetQt* parent_gtk = static_cast<WidgetQt*>(parent);
  QWidget* native_parent = NULL;
  if (parent != NULL) {
    if (type_ != TYPE_CHILD) {
      // window's parent has to be window.
      native_parent = parent_gtk->GetNativeView();
    } else {
      native_parent = parent_gtk->window_contents();
    }
  }
  Init(native_parent, bounds);
}

void WidgetQt::Init(QWidget* parent,
                     const gfx::Rect& bounds) {
  if (type_ != TYPE_CHILD)
    ActiveWindowWatcherX::AddObserver(this);
  // Force creation of the RootView if it hasn't been created yet.
  GetRootView();

//  default_theme_provider_.reset(new DefaultThemeProvider());

  // Make container here.
  CreateGtkWidget(parent, bounds);

  if (opacity_ != 255)
    SetOpacity(opacity_);

  // Make sure we receive our motion events.

  // In general we register most events on the parent of all widgets. At a
  // minimum we need painting to happen on the parent (otherwise painting
  // doesn't work at all), and similarly we need mouse release events on the
  // parent as windows don't get mouse releases.
  gtk_widget_add_events(widget_,
                        GDK_ENTER_NOTIFY_MASK |
                        GDK_LEAVE_NOTIFY_MASK |
                        GDK_BUTTON_PRESS_MASK |
                        GDK_BUTTON_RELEASE_MASK |
                        GDK_POINTER_MOTION_MASK |
                        GDK_KEY_PRESS_MASK |
                        GDK_KEY_RELEASE_MASK);
  SetRootViewForWidget(widget_, root_view_.get());

  g_signal_connect_after(G_OBJECT(widget_), "size_request",
                         G_CALLBACK(&OnSizeRequestThunk), this);
  g_signal_connect_after(G_OBJECT(widget_), "size_allocate",
                         G_CALLBACK(&OnSizeAllocateThunk), this);
  gtk_widget_set_app_paintable(widget_, true);
  g_signal_connect(widget_, "expose_event",
                   G_CALLBACK(&OnPaintThunk), this);
  g_signal_connect(widget_, "enter_notify_event",
                   G_CALLBACK(&OnEnterNotifyThunk), this);
  g_signal_connect(widget_, "leave_notify_event",
                   G_CALLBACK(&OnLeaveNotifyThunk), this);
  g_signal_connect(widget_, "motion_notify_event",
                   G_CALLBACK(&OnMotionNotifyThunk), this);
  g_signal_connect(widget_, "button_press_event",
                   G_CALLBACK(&OnButtonPressThunk), this);
  g_signal_connect(widget_, "button_release_event",
                   G_CALLBACK(&OnButtonReleaseThunk), this);
  g_signal_connect(widget_, "grab_broken_event",
                   G_CALLBACK(&OnGrabBrokeEventThunk), this);
  g_signal_connect(widget_, "grab_notify",
                   G_CALLBACK(&OnGrabNotifyThunk), this);
  g_signal_connect(widget_, "scroll_event",
                   G_CALLBACK(&OnScrollThunk), this);
  g_signal_connect(widget_, "visibility_notify_event",
                   G_CALLBACK(&OnVisibilityNotifyThunk), this);

  // In order to receive notification when the window is no longer the front
  // window, we need to install these on the widget.
  // NOTE: this doesn't work with focus follows mouse.
  g_signal_connect(widget_, "focus_in_event",
                   G_CALLBACK(&OnFocusInThunk), this);
  g_signal_connect(widget_, "focus_out_event",
                   G_CALLBACK(&OnFocusOutThunk), this);
  g_signal_connect(widget_, "destroy",
                   G_CALLBACK(&OnDestroyThunk), this);
  g_signal_connect(widget_, "show",
                   G_CALLBACK(&OnShowThunk), this);
  g_signal_connect(widget_, "hide",
                   G_CALLBACK(&OnHideThunk), this);

  // Views/FocusManager (re)sets the focus to the root window,
  // so we need to connect signal handlers to the gtk window.
  // See views::Views::Focus and views::FocusManager::ClearNativeFocus
  // for more details.
  g_signal_connect(widget_, "key_press_event",
                   G_CALLBACK(&OnKeyEventThunk), this);
  g_signal_connect(widget_, "key_release_event",
                   G_CALLBACK(&OnKeyEventThunk), this);

  // Drag and drop.
  gtk_drag_dest_set(widget_, static_cast<GtkDestDefaults>(0),
                    NULL, 0, GDK_ACTION_COPY);
  g_signal_connect(widget_, "drag_motion",
                   G_CALLBACK(&OnDragMotionThunk), this);
  g_signal_connect(widget_, "drag_data_received",
                   G_CALLBACK(&OnDragDataReceivedThunk), this);
  g_signal_connect(widget_, "drag_drop",
                   G_CALLBACK(&OnDragDropThunk), this);
  g_signal_connect(widget_, "drag_leave",
                   G_CALLBACK(&OnDragLeaveThunk), this);
  g_signal_connect(widget_, "drag_data_get",
                   G_CALLBACK(&OnDragDataGetThunk), this);
  g_signal_connect(widget_, "drag_end",
                   G_CALLBACK(&OnDragEndThunk), this);
  g_signal_connect(widget_, "drag_failed",
                   G_CALLBACK(&OnDragFailedThunk), this);

  tooltip_manager_.reset(new TooltipManagerGtk(this));

  // Register for tooltips.
  g_object_set(G_OBJECT(widget_), "has-tooltip", TRUE, NULL);
  g_signal_connect(widget_, "query_tooltip",
                   G_CALLBACK(&OnQueryTooltipThunk), this);

  if (type_ == TYPE_CHILD) {
    if (parent) {
      SetBounds(bounds);
    }
  } else {
    if (bounds.width() > 0 && bounds.height() > 0)
      gtk_window_resize(GTK_WINDOW(widget_), bounds.width(), bounds.height());
    gtk_window_move(GTK_WINDOW(widget_), bounds.x(), bounds.y());
  }
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
  if (!widget_) {
    // Due to timing we can get a request for the bounds after Close.
    *out = gfx::Rect(gfx::Point(0, 0), size_);
    return;
  }

  int x = 0, y = 0, w, h;
  if (GTK_IS_WINDOW(widget_)) {
    gtk_window_get_position(GTK_WINDOW(widget_), &x, &y);
    // NOTE: this doesn't include frame decorations, but it should be good
    // enough for our uses.
    gtk_window_get_size(GTK_WINDOW(widget_), &w, &h);
  } else {
    GetWidgetPositionOnScreen(widget_, &x, &y);
    w = widget_->allocation.width;
    h = widget_->allocation.height;
  }

  return out->SetRect(x, y, w, h);
}

void WidgetQt::SetBounds(const gfx::Rect& bounds) {
  if (type_ == TYPE_CHILD) {
    QWidget* parent = gtk_widget_get_parent(widget_);
    if (GTK_IS_VIEWS_FIXED(parent)) {
      WidgetQt* parent_widget = GetViewForNative(parent);
      parent_widget->PositionChild(widget_, bounds.x(), bounds.y(),
                                   bounds.width(), bounds.height());
    } else {
      DCHECK(GTK_IS_FIXED(parent))
          << "Parent of WidgetQt has to be Fixed or ViewsFixed";
      // Just request the size if the parent is not WidgetQt but plain
      // GtkFixed. WidgetQt does not know the minimum size so we assume
      // the caller of the SetBounds knows exactly how big it wants to be.
      gtk_widget_set_size_request(widget_, bounds.width(), bounds.height());
      if (parent != null_parent_)
        gtk_fixed_move(GTK_FIXED(parent), widget_, bounds.x(), bounds.y());
    }
  } else {
    if (GTK_WIDGET_MAPPED(widget_)) {
      // If the widget is mapped (on screen), we can move and resize with one
      // call, which avoids two separate window manager steps.
      gdk_window_move_resize(widget_->window, bounds.x(), bounds.y(),
                             bounds.width(), bounds.height());
    }

    // Always call gtk_window_move and gtk_window_resize so that GtkWindow's
    // geometry info is up-to-date.
    GtkWindow* gtk_window = GTK_WINDOW(widget_);
    // TODO: this may need to set an initial size if not showing.
    // TODO: need to constrain based on screen size.
    if (!bounds.IsEmpty()) {
      gtk_window_resize(gtk_window, bounds.width(), bounds.height());
    }
    gtk_window_move(gtk_window, bounds.x(), bounds.y());
  }
}

void WidgetQt::MoveAbove(Widget* widget) {
  DCHECK(widget_);
  DCHECK(widget_->window);
  // TODO(oshima): gdk_window_restack is not available in gtk2.0, so
  // we're simply raising the window to the top. We should switch to
  // gdk_window_restack when we upgrade gtk to 2.18 or up.
  gdk_window_raise(widget_->window);
}

void WidgetQt::SetShape(gfx::NativeRegion region) {
  DCHECK(widget_);
  DCHECK(widget_->window);
  gdk_window_shape_combine_region(widget_->window, region, 0, 0);
  gdk_region_destroy(region);
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
    gtk_widget_destroy(widget_);  // Triggers OnDestroy().
  }
}

void WidgetQt::Show() {
  if (widget_) {
    gtk_widget_show(widget_);
    if (widget_->window)
      gdk_window_raise(widget_->window);
  }
}

void WidgetQt::Hide() {
  if (widget_) {
    gtk_widget_hide(widget_);
    if (widget_->window)
      gdk_window_lower(widget_->window);
  }
}

gfx::NativeView WidgetQt::GetNativeView() const {
  return widget_;
}

void WidgetQt::PaintNow(const gfx::Rect& update_rect) {
  if (widget_ && GTK_WIDGET_DRAWABLE(widget_)) {
    gtk_widget_queue_draw_area(widget_, update_rect.x(), update_rect.y(),
                               update_rect.width(), update_rect.height());
    // Force the paint to occur now.
    AutoReset<bool> auto_reset_in_paint_now(&in_paint_now_, true);
    gdk_window_process_updates(widget_->window, true);
  }
}

void WidgetQt::SetOpacity(unsigned char opacity) {
  opacity_ = opacity;
  if (widget_) {
    // We can only set the opacity when the widget has been realized.
    gdk_window_set_opacity(widget_->window, static_cast<gdouble>(opacity) /
                           static_cast<gdouble>(255));
  }
}

void WidgetQt::SetAlwaysOnTop(bool on_top) {
  DCHECK(type_ != TYPE_CHILD);
  always_on_top_ = on_top;
  if (widget_)
    gtk_window_set_keep_above(GTK_WINDOW(widget_), on_top);
}

RootView* WidgetQt::GetRootView() {
  if (!root_view_.get()) {
    // First time the root view is being asked for, create it now.
    root_view_.reset(CreateRootView());
  }
  return root_view_.get();
}

Widget* WidgetQt::GetRootWidget() const {
  QWidget* parent = widget_;
  QWidget* last_parent = parent;
  while (parent) {
    last_parent = parent;
    parent = gtk_widget_get_parent(parent);
  }
  return last_parent ? GetViewForNative(last_parent) : NULL;
}

bool WidgetQt::IsVisible() const {
  return GTK_WIDGET_VISIBLE(widget_);
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
  return tooltip_manager_.get();
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
  if (drop_target_.get())
    drop_target_->ResetTargetViewIfEquals(child);
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
  if (!focus_manager_)
    return false;

  KeyEvent key(event);
  int key_code = key.GetKeyCode();
  bool handled = false;

  // Always reset |should_handle_menu_key_release_| unless we are handling a
  // VKEY_MENU key release event. It ensures that VKEY_MENU accelerator can only
  // be activated when handling a VKEY_MENU key release event which is preceded
  // by an unhandled VKEY_MENU key press event.
  if (key_code != ui::VKEY_MENU || event->type != GDK_KEY_RELEASE)
    should_handle_menu_key_release_ = false;

  if (event->type == GDK_KEY_PRESS) {
    // VKEY_MENU is triggered by key release event.
    // FocusManager::OnKeyEvent() returns false when the key has been consumed.
    if (key_code != ui::VKEY_MENU)
      handled = !focus_manager_->OnKeyEvent(key);
    else
      should_handle_menu_key_release_ = true;
  } else if (key_code == ui::VKEY_MENU && should_handle_menu_key_release_ &&
             (key.GetFlags() & ~Event::EF_ALT_DOWN) == 0) {
    // Trigger VKEY_MENU when only this key is pressed and released, and both
    // press and release events are not handled by others.
    Accelerator accelerator(ui::VKEY_MENU, false, false, false);
    handled = focus_manager_->ProcessAccelerator(accelerator);
  }

  return handled;
}

// static
int WidgetQt::GetFlagsForEventButton(const GdkEventButton& event) {
  int flags = Event::GetFlagsFromGdkState(event.state);
  switch (event.button) {
    case 1:
      flags |= Event::EF_LEFT_BUTTON_DOWN;
      break;
    case 2:
      flags |= Event::EF_MIDDLE_BUTTON_DOWN;
      break;
    case 3:
      flags |= Event::EF_RIGHT_BUTTON_DOWN;
      break;
    default:
      // We only deal with 1-3.
      break;
  }
  if (event.type == GDK_2BUTTON_PRESS)
    flags |= MouseEvent::EF_IS_DOUBLE_CLICK;
  return flags;
}

////////////////////////////////////////////////////////////////////////////////
// WidgetQt, protected:

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

void WidgetQt::DoGrab() {
  has_capture_ = true;
  gtk_grab_add(widget_);
}

void WidgetQt::ReleaseGrab() {
  if (has_capture_) {
    has_capture_ = false;
    gtk_grab_remove(widget_);
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

gboolean WidgetQt::OnWindowPaint(QWidget* widget, GdkEventExpose* event) {
  // Clear the background to be totally transparent. We don't need to
  // paint the root view here as that is done by OnPaint.
  DCHECK(transparent_);
  DrawTransparentBackground(widget, event);
  return false;
}

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
  // An event may come from a contained widget which has its own gdk window.
  // Translate it to the widget's coordinates.
  int x = 0, y = 0;
  GetContainedWidgetEventCoordinates(event, &x, &y);
  int flags = Event::GetFlagsFromGdkState(event->state);
  int increment = 0;
  bool is_horizontal = true;
  switch (event->direction) {
    case GDK_SCROLL_UP:
      increment = -1;
      is_horizontal = false;
      break;
    case GDK_SCROLL_DOWN:
      increment = 1;
      is_horizontal = false;
      break;
    case GDK_SCROLL_LEFT:
      increment = -1;
      is_horizontal = true;
      break;
    case GDK_SCROLL_RIGHT:
      increment = 1;
      is_horizontal = false;
      break;
  }
  increment *= is_horizontal ? root_view_->width() / 5 :
      root_view_->height() / 5;
  MouseWheelEvent wheel_event(increment, x, y, flags);
  return root_view_->ProcessMouseWheelEvent(wheel_event);
}

// static
void WidgetQt::SetRootViewForWidget(QWidget* widget, RootView* root_view) {
  g_object_set_data(G_OBJECT(widget), "root-view", root_view);
}

// static
Window* WidgetQt::GetWindowImpl(QWidget* widget) {
  QWidget* parent = widget;
  while (parent) {
    WidgetQt* widget_gtk = GetViewForNative(parent);
    if (widget_gtk && widget_gtk->is_window_)
      return static_cast<WindowGtk*>(widget_gtk);
    parent = gtk_widget_get_parent(parent);
  }
  return NULL;
}

void WidgetQt::CreateQtWidget(QWidget* parent, const gfx::Rect& bounds) {
  if (type_ == TYPE_CHILD) {
    widget_ = new QGroupBox();
  } else {
    widget_ = new QMainWindow();
  }
  layout_ = new QLayout(widget_);
}

void WidgetQt::ConfigureWidgetForTransparentBackground(QWidget* parent) {
  DCHECK(widget_ && widget_);

  GdkColormap* rgba_colormap =
      gdk_screen_get_rgba_colormap(gtk_widget_get_screen(widget_));
  if (!rgba_colormap) {
    transparent_ = false;
    return;
  }
  // To make the background transparent we need to install the RGBA colormap
  // on both the window and fixed. In addition we need to make sure no
  // decorations are drawn. The last bit is to make sure the widget doesn't
  // attempt to draw a pixmap in it's background.
  if (type_ != TYPE_CHILD) {
    DCHECK(parent == NULL);
    gtk_widget_set_colormap(widget_, rgba_colormap);
    gtk_widget_set_app_paintable(widget_, true);
    g_signal_connect(widget_, "expose_event",
                     G_CALLBACK(&OnWindowPaintThunk), this);
    gtk_widget_realize(widget_);
    gdk_window_set_decorations(widget_->window,
                               static_cast<GdkWMDecoration>(0));
  } else {
    DCHECK(parent);
    CompositePainter::AddCompositePainter(parent);
  }
  DCHECK(!GTK_WIDGET_REALIZED(widget_));
  gtk_widget_set_colormap(widget_, rgba_colormap);
}

void WidgetQt::ConfigureWidgetForIgnoreEvents() {
  gtk_widget_realize(widget_);
  GdkWindow* gdk_window = widget_->window;
  Display* display = GDK_WINDOW_XDISPLAY(gdk_window);
  XID win = GDK_WINDOW_XID(gdk_window);

  // This sets the clickable area to be empty, allowing all events to be
  // passed to any windows behind this one.
  XShapeCombineRectangles(
      display,
      win,
      ShapeInput,
      0,  // x offset
      0,  // y offset
      NULL,  // rectangles
      0,  // num rectangles
      ShapeSet,
      0);
}

void WidgetQt::DrawTransparentBackground(QWidget* widget,
                                          GdkEventExpose* event) {
  cairo_t* cr = gdk_cairo_create(widget->window);
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  gdk_cairo_region(cr, event->region);
  cairo_fill(cr);
  cairo_destroy(cr);
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

// Callback from gtk_container_foreach. Locates the first root view of widget
// or one of it's descendants.
static void RootViewLocatorCallback(QWidget* widget,
                                    gpointer root_view_p) {
  RootView** root_view = static_cast<RootView**>(root_view_p);
  if (!*root_view) {
    *root_view = WidgetQt::GetRootViewForWidget(widget);
    if (!*root_view && GTK_IS_CONTAINER(widget)) {
      // gtk_container_foreach only iterates over children, not all descendants,
      // so we have to recurse here to get all descendants.
      gtk_container_foreach(GTK_CONTAINER(widget), RootViewLocatorCallback,
                            root_view_p);
    }
  }
}

// static
RootView* Widget::FindRootView(GtkWindow* window) {
  RootView* root_view = WidgetQt::GetRootViewForWidget(GTK_WIDGET(window));
  if (root_view)
    return root_view;

  // Enumerate all children and check if they have a RootView.
  gtk_container_foreach(GTK_CONTAINER(window), RootViewLocatorCallback,
                        static_cast<gpointer>(&root_view));
  return root_view;
}

static void AllRootViewsLocatorCallback(QWidget* widget,
                                        gpointer root_view_p) {
  std::set<RootView*>* root_views_set =
      reinterpret_cast<std::set<RootView*>*>(root_view_p);
  RootView *root_view = WidgetQt::GetRootViewForWidget(widget);
  if (!root_view && GTK_IS_CONTAINER(widget)) {
    // gtk_container_foreach only iterates over children, not all descendants,
    // so we have to recurse here to get all descendants.
    gtk_container_foreach(GTK_CONTAINER(widget), AllRootViewsLocatorCallback,
        root_view_p);
  } else {
    if (root_view)
      root_views_set->insert(root_view);
  }
}

// static
void Widget::FindAllRootViews(GtkWindow* window,
                              std::vector<RootView*>* root_views) {
  RootView* root_view = WidgetQt::GetRootViewForWidget(GTK_WIDGET(window));
  if (root_view)
    root_views->push_back(root_view);

  std::set<RootView*> root_views_set;

  // Enumerate all children and check if they have a RootView.
  gtk_container_foreach(GTK_CONTAINER(window), AllRootViewsLocatorCallback,
      reinterpret_cast<gpointer>(&root_views_set));
  root_views->clear();
  root_views->reserve(root_views_set.size());
  for (std::set<RootView*>::iterator it = root_views_set.begin();
      it != root_views_set.end();
      ++it)
    root_views->push_back(*it);
}

// static
Widget* Widget::GetWidgetFromNativeView(gfx::NativeView native_view) {
  gpointer raw_widget = g_object_get_data(G_OBJECT(native_view), kWidgetKey);
  if (raw_widget)
    return reinterpret_cast<Widget*>(raw_widget);
  return NULL;
}

// static
Widget* Widget::GetWidgetFromNativeWindow(gfx::NativeWindow native_window) {
  gpointer raw_widget = g_object_get_data(G_OBJECT(native_window), kWidgetKey);
  if (raw_widget)
    return reinterpret_cast<Widget*>(raw_widget);
  return NULL;
}

// static
void Widget::NotifyLocaleChanged() {
  GList *window_list = gtk_window_list_toplevels();
  for (GList* element = window_list; element; element = g_list_next(element)) {
    GtkWindow* window = GTK_WINDOW(element->data);
    DCHECK(window);
    RootView *root_view = FindRootView(window);
    if (root_view)
      root_view->NotifyLocaleChanged();
  }
  g_list_free(window_list);
}

}  // namespace views

