QWT_ROOT = $${PWD}/../../../qwt

QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = QCTools
TEMPLATE = app

CONFIG += qt release
CONFIG += no_keywords

HEADERS = \
    ../../Source/GUI/mainwindow.h \
    ../../Source/GUI/Help.h \
    ../../Source/Core/Core.h

SOURCES = \
    ../../Source/GUI/main.cpp \
    ../../Source/GUI/mainwindow.cpp \
    ../../Source/GUI/mainwindow_More.cpp \
    ../../Source/GUI/Help.cpp \
    ../../Source/Core/Core.cpp

FORMS += \
    ../../Source/GUI/mainwindow.ui \

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
