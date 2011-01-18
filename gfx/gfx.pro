#-------------------------------------------------
#
# Project created by QtCreator 2011-01-18T14:41:51
#
#-------------------------------------------------

QT       -= core gui

TARGET = gfx
TEMPLATE = lib
CONFIG += staticlib

SOURCES += \
    win_util.cc \
    test_suite.cc \
    skia_utils_gtk.cc \
    skia_util.cc \
    skbitmap_operations_unittest.cc \
    skbitmap_operations.cc \
    size.cc \
    scrollbar_size.cc \
    scoped_image_unittest.cc \
    run_all_unittests.cc \
    rect_unittest.cc \
    rect.cc \
    point.cc \
    platform_font_win.cc \
    platform_font_gtk.cc \
    path_win.cc \
    path_gtk.cc \
    path.cc \
    native_widget_types_gtk.cc \
    native_theme_win_unittest.cc \
    native_theme_win.cc \
    native_theme_linux.cc \
    insets_unittest.cc \
    insets.cc \
    icon_util_unittest.cc \
    icon_util.cc \
    gtk_util.cc \
    gtk_preserve_window.cc \
    gtk_native_view_id_manager.cc \
    gfx_paths.cc \
    gdi_util.cc \
    font_unittest.cc \
    font.cc \
    empty.cc \
    color_utils_unittest.cc \
    color_utils.cc \
    canvas_skia_win.cc \
    canvas_skia_linux.cc \
    canvas_skia.cc \
    canvas_direct2d_unittest.cc \
    canvas_direct2d.cc \
    canvas.cc \
    blit_unittest.cc \
    blit.cc

HEADERS += \
    win_util.h \
    test_suite.h \
    skia_utils_gtk.h \
    skia_util.h \
    skbitmap_operations.h \
    size.h \
    scrollbar_size.h \
    scoped_image.h \
    scoped_cg_context_state_mac.h \
    rect.h \
    point.h \
    platform_font_win.h \
    platform_font_mac.h \
    platform_font.h \
    platform_font_gtk.h \
    path.h \
    native_widget_types.h \
    native_theme_win.h \
    native_theme_linux.h \
    insets.h \
    icon_util.h \
    gtk_util.h \
    gtk_preserve_window.h \
    gtk_native_view_id_manager.h \
    gfx_paths.h \
    gdi_util.h \
    font.h \
    favicon_size.h \
    color_utils.h \
    canvas_skia_paint.h \
    canvas_skia.h \
    canvas.h \
    canvas_direct2d.h \
    brush.h \
    blit.h
unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/local/lib
    }
    INSTALLS += target
}

OTHER_FILES += \
    platform_font_mac.mm \
    gfx_unittests.target.mk \
    gfx.target.mk \
    gfx_resources.target.mk \
    gfx_resources.grd \
    gfx.pro.user \
    gfx.Makefile \
    gfx.gyp \
    DEPS \
    canvas_skia_mac.mm
