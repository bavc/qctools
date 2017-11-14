QT = core network

TARGET = qctools
TEMPLATE = lib
CONFIG += c++11
CONFIG += staticlib

include(../brew.pri)
include(../ffmpeg.pri)

SOURCES_PATH = $$absolute_path($$PWD/../../../Source)
message("qctools-lib: SOURCES_PATH = " $$SOURCES_PATH)

THIRD_PARTY_PATH = $$absolute_path($$SOURCES_PATH/../..)
message("qctools-lib: THIRD_PARTY_PATH = " $$THIRD_PARTY_PATH)

INCLUDEPATH += $$SOURCES_PATH
INCLUDEPATH += $$SOURCES_PATH/ThirdParty/tinyxml2

QMAKE_CXXFLAGS += -DWITH_SYSTEM_FFMPEG=1

HEADERS = \
    $$SOURCES_PATH/ThirdParty/tinyxml2/tinyxml2.h \
    $$SOURCES_PATH/Core/AudioCore.h \
    $$SOURCES_PATH/Core/AudioStats.h \
    $$SOURCES_PATH/Core/CommonStats.h \
    $$SOURCES_PATH/Core/Core.h \
    $$SOURCES_PATH/Core/FFmpeg_Glue.h \
    $$SOURCES_PATH/Core/VideoCore.h \
    $$SOURCES_PATH/Core/VideoStats.h \
    $$SOURCES_PATH/Core/FormatStats.h \
    $$SOURCES_PATH/Core/CommonStreamStats.h \
    $$SOURCES_PATH/Core/AudioStreamStats.h \
    $$SOURCES_PATH/Core/VideoStreamStats.h \
    $$SOURCES_PATH/Core/StreamsStats.h \
    $$SOURCES_PATH/Core/Timecode.h \
    $$SOURCES_PATH/Core/FileInformation.h \
    $$SOURCES_PATH/Core/SignalServerConnectionChecker.h \
    $$SOURCES_PATH/Core/SignalServer.h \
    $$SOURCES_PATH/Core/Preferences.h


SOURCES = \
    $$SOURCES_PATH/ThirdParty/tinyxml2/tinyxml2.cpp \
    $$SOURCES_PATH/Core/AudioCore.cpp \
    $$SOURCES_PATH/Core/AudioStats.cpp \
    $$SOURCES_PATH/Core/CommonStats.cpp \
    $$SOURCES_PATH/Core/Core.cpp \
    $$SOURCES_PATH/Core/FFmpeg_Glue.cpp \
    $$SOURCES_PATH/Core/VideoCore.cpp \
    $$SOURCES_PATH/Core/VideoStats.cpp \
    $$SOURCES_PATH/Core/FormatStats.cpp \
    $$SOURCES_PATH/Core/CommonStreamStats.cpp \
    $$SOURCES_PATH/Core/AudioStreamStats.cpp \
    $$SOURCES_PATH/Core/VideoStreamStats.cpp \
    $$SOURCES_PATH/Core/StreamsStats.cpp \
    $$SOURCES_PATH/Core/Timecode.cpp \
    $$SOURCES_PATH/Core/FileInformation.cpp \
    $$SOURCES_PATH/Core/SignalServerConnectionChecker.cpp \
    $$SOURCES_PATH/Core/SignalServer.cpp \
    $$SOURCES_PATH/Core/Preferences.cpp

win32 {
    greaterThan(QT_MAJOR_VERSION, 4): {
        greaterThan(QT_MINOR_VERSION, 8): {
            ZLIB_INCLUDE_PATH = $$absolute_path($$[QT_INSTALL_PREFIX]/../src/qtbase/src/3rdparty/zlib/src)
        } else {
            ZLIB_INCLUDE_PATH = $$absolute_path($$[QT_INSTALL_PREFIX]/../src/qtbase/src/3rdparty/zlib)
        }
    }
    message("qctools-lib: ZLIB_INCLUDE_PATH = " $$ZLIB_INCLUDE_PATH)
    INCLUDEPATH += $$ZLIB_INCLUDE_PATH
}

include(../blackmagic.pri)
include($$SOURCES_PATH/ThirdParty/qblowfish/qblowfish.pri)
