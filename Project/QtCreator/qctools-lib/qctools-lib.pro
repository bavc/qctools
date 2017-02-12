QT = core gui

TARGET = qctools
TEMPLATE = lib
CONFIG += c++11
CONFIG += staticlib

include(../ffmpeg.pri)

SOURCES_PATH = $$PWD/../../../Source
message("qctools: SOURCES_PATH = " $$absolute_path($$SOURCES_PATH))

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
    $$SOURCES_PATH/Core/Timecode.h \


SOURCES = \
    $$SOURCES_PATH/ThirdParty/tinyxml2/tinyxml2.cpp \
    $$SOURCES_PATH/Core/AudioCore.cpp \
    $$SOURCES_PATH/Core/AudioStats.cpp \
    $$SOURCES_PATH/Core/CommonStats.cpp \
    $$SOURCES_PATH/Core/Core.cpp \
    $$SOURCES_PATH/Core/FFmpeg_Glue.cpp \
    $$SOURCES_PATH/Core/VideoCore.cpp \
    $$SOURCES_PATH/Core/VideoStats.cpp \
    $$SOURCES_PATH/Core/Timecode.cpp \
