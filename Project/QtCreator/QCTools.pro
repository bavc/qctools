QWT_ROOT = $${PWD}/../../../qwt

QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = QCTools
TEMPLATE = app

CONFIG += qt release
CONFIG += no_keywords

HEADERS = \
    ../../Source/Core/Core.h \
    ../../Source/Core/FFmpeg_Glue.h \
    ../../Source/GUI/BigDisplay.h \
    ../../Source/GUI/Control.h \
    ../../Source/GUI/FileInformation.h \
    ../../Source/GUI/FilesList.h \
    ../../Source/GUI/Help.h \
    ../../Source/GUI/Info.h \
    ../../Source/GUI/mainwindow.h \
    ../../Source/GUI/Plots.h \
    ../../Source/GUI/TinyDisplay.h

SOURCES = \
    ../../Source/Core/Core.cpp \
    ../../Source/Core/FFmpeg_Glue.cpp \
    ../../Source/GUI/BigDisplay.cpp \
    ../../Source/GUI/Control.cpp \
    ../../Source/GUI/FileInformation.cpp \
    ../../Source/GUI/FilesList.cpp \
    ../../Source/GUI/Help.cpp \
    ../../Source/GUI/Info.cpp \
    ../../Source/GUI/main.cpp \
    ../../Source/GUI/mainwindow.cpp \
    ../../Source/GUI/mainwindow_Callbacks.cpp \
    ../../Source/GUI/mainwindow_More.cpp \
    ../../Source/GUI/mainwindow_Ui.cpp \
    ../../Source/GUI/Plots.cpp \
    ../../Source/GUI/TinyDisplay.cpp

FORMS += \
    ../../Source/GUI/mainwindow.ui

RESOURCES += \
    ../../Source/Resource/Resources.qrc

include( $${QWT_ROOT}/qwtconfig.pri )
include( $${QWT_ROOT}/qwtbuild.pri )
include( $${QWT_ROOT}/qwtfunctions.pri )

INCLUDEPATH += $$PWD/../../Source
INCLUDEPATH += $$QWT_ROOT/src
INCLUDEPATH += $$PWD/../../../FFmpeg-bavc

LIBS      += -L$${QWT_ROOT}/lib -lqwt
LIBS      += -L$${PWD}/../../../FFmpeg-bavc/libavdevice -lavdevice \
             -L$${PWD}/../../../FFmpeg-bavc/libavcodec -lavcodec \
             -L$${PWD}/../../../FFmpeg-bavc/libavfilter -lavfilter \
             -L$${PWD}/../../../FFmpeg-bavc/libavformat -lavformat \
             -L$${PWD}/../../../FFmpeg-bavc/libavutil -lavutil \
             -L$${PWD}/../../../FFmpeg-bavc/libpostproc -lpostproc \
             -L$${PWD}/../../../FFmpeg-bavc/libswresample -lswresample \
             -L$${PWD}/../../../FFmpeg-bavc/libswscale -lswscale

macx:
{
    ICON = ../../Source/Resource/Logo.icns 
}