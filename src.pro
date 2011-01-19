#-------------------------------------------------
#
# Project created by QtCreator 2011-01-19T15:16:25
#
#-------------------------------------------------

QT       -= core gui

TARGET = src
TEMPLATE = lib
CONFIG += staticlib

SOURCES +=

HEADERS += \
    build/build_config.h
unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/local/lib
    }
    INSTALLS += target
}

OTHER_FILES += \
    build/common.gypi \
    app/app_base.gypi
