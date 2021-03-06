// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GFX_GTK_PRESERVE_WINDOW_H_
#define GFX_GTK_PRESERVE_WINDOW_H_
#pragma once

#include <gdk/gdk.h>
#include <gtk/gtk.h>

// GtkFixed creates an X window when realized and destroys an X window
// when unrealized. GtkPreserveWindow allows overrides this
// behaviour. When preserve is set (via gtk_preserve_window_set_preserve),
// the X window is only destroyed when the widget is destroyed.

G_BEGIN_DECLS

#define GTK_TYPE_PRESERVE_WINDOW                                 \
    (gtk_preserve_window_get_type())
#define GTK_PRESERVE_WINDOW(obj)                                 \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GTK_TYPE_PERSERVE_WINDOW, \
                                GtkPreserveWindow))
#define GTK_PRESERVE_WINDOW_CLASS(klass)                         \
    (G_TYPE_CHECK_CLASS_CAST((klass), GTK_TYPE_PRESERVE_WINDOW,  \
                             GtkPreserveWindowClass))
#define GTK_IS_PRESERVE_WINDOW(obj)                              \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GTK_TYPE_PRESERVE_WINDOW))
#define GTK_IS_PRESERVE_WINDOW_CLASS(klass)                      \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GTK_TYPE_PRESERVE_WINDOW))
#define GTK_PRESERVE_WINDOW_GET_CLASS(obj)                       \
    (G_TYPE_INSTANCE_GET_CLASS((obj), GTK_TYPE_PRESERVE_WINDOW,  \
                               GtkPreserveWindowClass))

typedef struct _GtkPreserveWindow GtkPreserveWindow;
typedef struct _GtkPreserveWindowClass GtkPreserveWindowClass;

struct _GtkPreserveWindow {
  // Parent class.
  GtkFixed fixed;
};

struct _GtkPreserveWindowClass {
  GtkFixedClass parent_class;
};

GType gtk_preserve_window_get_type() G_GNUC_CONST;
GtkWidget* gtk_preserve_window_new();

// Whether or not we should preserve associated windows as the widget
// is realized or unrealized.
gboolean gtk_preserve_window_get_preserve(GtkPreserveWindow* widget);
void gtk_preserve_window_set_preserve(GtkPreserveWindow* widget,
                                      gboolean value);

// Whether or not someone else will gdk_window_resize the GdkWindow associated
// with this widget (needed by the GPU process to synchronize resizing
// with swapped between front and back buffer).
void gtk_preserve_window_delegate_resize(GtkPreserveWindow* widget,
                                         gboolean delegate);

G_END_DECLS

#endif  // GFX_GTK_PRESERVE_WINDOW_H_
