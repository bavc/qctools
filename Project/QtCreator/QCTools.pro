QWT_ROOT = $${PWD}/../../../qwt

QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = QCTools
TEMPLATE = app

CONFIG += qt release
CONFIG += no_keywords

HEADERS = \
    ../../Source/Core/Core.h \
    ../../Source/Core/ffmpeg_BasicInfo.h \
    ../../Source/Core/ffmpeg_Pictures.h \
    ../../Source/Core/ffmpeg_Stats.h \
    ../../Source/Core/ffmpeg_Thumbnails.h \
    ../../Source/GUI/mainwindow.h \
    ../../Source/GUI/Help.h \
    ../../Source/GUI/PerPicture.h

SOURCES = \
    ../../Source/Core/Core.cpp \
    ../../Source/Core/ffmpeg_BasicInfo.cpp \
    ../../Source/Core/ffmpeg_Pictures.cpp \
    ../../Source/Core/ffmpeg_Stats.cpp \
    ../../Source/Core/ffmpeg_Thumbnails.cpp \
    ../../Source/GUI/Help.cpp \
    ../../Source/GUI/main.cpp \
    ../../Source/GUI/mainwindow.cpp \
    ../../Source/GUI/mainwindow_Callbacks.cpp \
    ../../Source/GUI/mainwindow_More.cpp \
    ../../Source/GUI/mainwindow_Pictures.cpp \
    ../../Source/GUI/mainwindow_Plots.cpp \
    ../../Source/GUI/mainwindow_Ui.cpp \
    ../../Source/GUI/PerFile.cpp \
    ../../Source/GUI/PerPicture.cpp

FORMS += \
    ../../Source/GUI/mainwindow.ui \
    ../../Source/GUI/PerPicture.ui

RESOURCES += \
    ../../Source/Resource/Resources.qrc

include( $${QWT_ROOT}/qwtconfig.pri )
include( $${QWT_ROOT}/qwtbuild.pri )
include( $${QWT_ROOT}/qwtfunctions.pri )

INCLUDEPATH += $$PWD/../../Source
INCLUDEPATH += $$QWT_ROOT/src
INCLUDEPATH += $$PWD/../../../ZenLib/Source

LIBS      += -L$${QWT_ROOT}/lib -lqwt
LIBS      += -L$${PWD}/../../../ZenLib/Project/GNU/Library/.libs -lzen

//win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../ZenLib/Project/MSVC2010/Library/Win32/release/ -lZenLib
//else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../ZenLib/Project/MSVC2010/Library/Win32/debug/ -lZenLib
//else:unix: LIBS += -L$$PWD/../../../ZenLib/Project/MSVC2010/Library/Win32/ -lZenLib

//
//DEPENDPATH += $$PWD/../../../ZenLib/Source
