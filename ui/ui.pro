#-------------------------------------------------
#
# Project created by QtCreator 2011-01-22T21:03:41
#
#-------------------------------------------------

QT       -= core gui

TARGET = ui
TEMPLATE = lib
CONFIG += staticlib

SOURCES += \
    base/dragdrop/os_exchange_data_win_unittest.cc \
    base/dragdrop/os_exchange_data_provider_win.cc \
    base/dragdrop/os_exchange_data_provider_gtk.cc \
    base/dragdrop/os_exchange_data.cc \
    base/dragdrop/gtk_dnd_util_unittest.cc \
    base/dragdrop/gtk_dnd_util.cc \
    base/dragdrop/drop_target.cc \
    base/dragdrop/drag_source.cc \
    base/dragdrop/drag_drop_types_win.cc \
    base/dragdrop/drag_drop_types_gtk.cc \
    base/dragdrop/os_exchange_data_provider_qt.cc

HEADERS += \
    base/dragdrop/os_exchange_data_provider_win.h \
    base/dragdrop/os_exchange_data_provider_gtk.h \
    base/dragdrop/os_exchange_data.h \
    base/dragdrop/gtk_dnd_util.h \
    base/dragdrop/drop_target.h \
    base/dragdrop/drag_source.h \
    base/dragdrop/drag_drop_types.h \
    base/dragdrop/download_file_interface.h

OTHER_FILES += \
    ui.gyp
