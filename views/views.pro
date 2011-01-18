#-------------------------------------------------
#
# Project created by QtCreator 2010-12-23T20:35:39
#
#-------------------------------------------------

QT       -= core gui

TARGET = views
TEMPLATE = lib

DEFINES += VIEWS_LIBRARY

SOURCES += \
    view.cc \
    view_win.cc \
    view_unittest.cc \
    view_text_utils.cc \
    view_gtk.cc \
    view_constants.cc \
    stubs.cc \
    screen_win.cc \
    screen_gtk.cc \
    run_all_unittests.cc \
    repeat_controller.cc \
    painter.cc \
    mouse_watcher.cc \
    layout_manager.cc \
    grid_layout.cc \
    grid_layout_unittest.cc \
    fill_layout.cc \
    event.cc \
    event_x.cc \
    event_win.cc \
    event_gtk.cc \
    drag_utils.cc \
    drag_utils_win.cc \
    drag_utils_gtk.cc \
    box_layout.cc \
    box_layout_unittest.cc \
    border.cc \
    background.cc \
    accelerator.cc \
    window/window.cc \
    window/window_win.cc \
    window/window_shape.cc \
    window/window_gtk.cc \
    window/window_delegate.cc \
    window/non_client_view.cc \
    window/native_frame_view.cc \
    window/dialog_delegate.cc \
    window/dialog_client_view.cc \
    window/custom_frame_view.cc \
    window/client_view.cc \
    widget/widget_win.cc \
    widget/widget_utils.cc \
    widget/widget_gtk.cc \
    widget/tooltip_window_gtk.cc \
    widget/tooltip_manager.cc \
    widget/tooltip_manager_win.cc \
    widget/tooltip_manager_gtk.cc \
    widget/root_view.cc \
    widget/root_view_win.cc \
    widget/root_view_gtk.cc \
    widget/gtk_views_window.cc \
    widget/gtk_views_fixed.cc \
    widget/drop_target_win.cc \
    widget/drop_target_gtk.cc \
    widget/drop_helper.cc \
    widget/default_theme_provider.cc \
    widget/child_window_message_processor.cc \
    widget/aero_tooltip_manager.cc \
    controls/tabbed_pane/tabbed_pane.cc \
    controls/tabbed_pane/tabbed_pane_unittest.cc \
    controls/tabbed_pane/native_tabbed_pane_win.cc \
    controls/tabbed_pane/native_tabbed_pane_gtk.cc \
    controls/button/text_button.cc \
    controls/button/radio_button.cc \
    controls/button/native_button.cc \
    controls/button/native_button_win.cc \
    controls/button/native_button_gtk.cc \
    controls/button/menu_button.cc \
    controls/button/image_button.cc \
    controls/button/custom_button.cc \
    controls/button/checkbox.cc \
    controls/button/button.cc \
    controls/button/button_dropdown.cc \
    controls/listbox/listbox.cc \
    controls/listbox/native_listbox_win.cc \
    widget/widget_qt.cc

HEADERS += \
    views_delegate.h \
    view.h \
    view_text_utils.h \
    view_constants.h \
    standard_layout.h \
    screen.h \
    repeat_controller.h \
    painter.h \
    mouse_watcher.h \
    layout_manager.h \
    grid_layout.h \
    fill_layout.h \
    event.h \
    drag_utils.h \
    box_layout.h \
    border.h \
    background.h \
    accelerator.h \
    window/window.h \
    window/window_win.h \
    window/window_shape.h \
    window/window_resources.h \
    window/window_gtk.h \
    window/window_delegate.h \
    window/non_client_view.h \
    window/native_frame_view.h \
    window/hit_test.h \
    window/dialog_delegate.h \
    window/dialog_client_view.h \
    window/custom_frame_view.h \
    window/client_view.h \
    widget/widget.h \
    widget/widget_win.h \
    widget/widget_utils.h \
    widget/widget_gtk.h \
    widget/widget_delegate.h \
    widget/tooltip_window_gtk.h \
    widget/tooltip_manager.h \
    widget/tooltip_manager_win.h \
    widget/tooltip_manager_gtk.h \
    widget/root_view.h \
    widget/gtk_views_window.h \
    widget/gtk_views_fixed.h \
    widget/drop_target_win.h \
    widget/drop_target_gtk.h \
    widget/drop_helper.h \
    widget/default_theme_provider.h \
    widget/child_window_message_processor.h \
    widget/aero_tooltip_manager.h \
    controls/tabbed_pane/tabbed_pane.h \
    controls/tabbed_pane/native_tabbed_pane_wrapper.h \
    controls/tabbed_pane/native_tabbed_pane_win.h \
    controls/tabbed_pane/native_tabbed_pane_gtk.h \
    controls/button/text_button.h \
    controls/button/radio_button.h \
    controls/button/native_button.h \
    controls/button/native_button_wrapper.h \
    controls/button/native_button_win.h \
    controls/button/native_button_gtk.h \
    controls/button/menu_button.h \
    controls/button/image_button.h \
    controls/button/custom_button.h \
    controls/button/checkbox.h \
    controls/button/button.h \
    controls/button/button_dropdown.h \
    controls/listbox/native_listbox_win.h \
    controls/listbox/listbox.h \
    controls/listbox/native_listbox_wrapper.h \
    widget/widget_qt.h

OTHER_FILES += \
    views.target.mk \
    views.pro.user \
    views.Makefile \
    views.gyp \
    views_unittests.target.mk \
    views_pack.Makefile \
    views_examples.target.mk \
    PRESUBMIT.py \
    packed_resources.target.mk \
    DEPS \
    widget/widget_gtk.cc~ \
    widget/drop_target_gtk.cc~
