message('entering qctools-cli.pro')

QT += core network
QT -= gui

CONFIG += c++1z

TARGET = qcli
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

message("PWD = " $$PWD)

win32:RC_FILE = qcli.rc

# link against libqctools
win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../qctools-lib/release/ -lqctools
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../qctools-lib/debug/ -lqctools
else:unix: LIBS += -L$$OUT_PWD/../qctools-lib/ -lqctools

INCLUDEPATH += $$PWD/../qctools-lib
DEPENDPATH += $$PWD/../qctools-lib

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../qctools-lib/release/libqctools.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../qctools-lib/debug/libqctools.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../qctools-lib/release/qctools.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../qctools-lib/debug/qctools.lib
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../qctools-lib/libqctools.a

SOURCES_PATH = $$PWD/../../../Source
message("qctools: SOURCES_PATH = " $$absolute_path($$SOURCES_PATH))

THIRD_PARTY_PATH = $$absolute_path($$SOURCES_PATH/../..)
message("qctools: THIRD_PARTY_PATH = " $$absolute_path($$THIRD_PARTY_PATH))

INCLUDEPATH += $$SOURCES_PATH

HEADERS += $$SOURCES_PATH/Cli/version.h \
           $$SOURCES_PATH/Cli/cli.h

SOURCES += $$SOURCES_PATH/Cli/main.cpp \
           $$SOURCES_PATH/Cli/cli.cpp


# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNING

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
include(../zlib.pri)

!win32 {
    LIBS      += -lbz2
}

unix {
    LIBS       += -lz -ldl
    !macx:LIBS += -lrt
}

macx:LIBS += -liconv \
             -framework CoreFoundation \
             -framework Foundation \
             -framework AppKit \
             -framework AudioToolbox \
             -framework QuartzCore \
             -framework CoreGraphics \
             -framework CoreAudio \
             -framework CoreVideo \
             -framework OpenGL \
             -framework VideoDecodeAcceleration

HEADERS += \
    version.h

INCLUDEPATH += ../qctools-QtAVPlayer/src
include(../qctools-QtAVPlayer/src/QtAVPlayer/QtAVPlayer.pri)

message('qctools-lib: including ffmpeg')
include(../ffmpeg.pri)

message('leaving qctools-cli.pro')
