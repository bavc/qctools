QWT_ROOT = $${PWD}/../../../qwt

QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = QCTools
TEMPLATE = app

CONFIG += qt debug_and_release
CONFIG += no_keywords
QMAKE_CXXFLAGS += -DBLACKMAGICDECKLINK_NO -DWITH_SYSTEM_FFMPEG=1

HEADERS = \
    ../../Source/Core/AudioCore.h \
    ../../Source/Core/AudioStats.h \
    ../../Source/Core/CommonStats.h \
    ../../Source/Core/Core.h \
    ../../Source/Core/BlackmagicDeckLink.h \
    ../../Source/Core/BlackmagicDeckLink_Glue.h \
    ../../Source/Core/FFmpeg_Glue.h \
    ../../Source/Core/VideoCore.h \
    ../../Source/Core/VideoStats.h \
    ../../Source/Core/Timecode.h \
    ../../Source/GUI/blackmagicdecklink_userinput.h \
    ../../Source/GUI/BigDisplay.h \
    ../../Source/GUI/Control.h \
    ../../Source/GUI/FileInformation.h \
    ../../Source/GUI/FilesList.h \
    ../../Source/GUI/Help.h \
    ../../Source/GUI/Info.h \
    ../../Source/GUI/mainwindow.h \
    ../../Source/GUI/preferences.h \
    ../../Source/GUI/Plot.h \
    ../../Source/GUI/Plots.h \
    ../../Source/GUI/PlotLegend.h \
    ../../Source/GUI/PlotScaleWidget.h \
    ../../Source/GUI/TinyDisplay.h \
    ../../Source/ThirdParty/tinyxml2/tinyxml2.h

SOURCES = \
    ../../Source/Core/AudioCore.cpp \
    ../../Source/Core/AudioStats.cpp \
    ../../Source/Core/CommonStats.cpp \
    ../../Source/Core/Core.cpp \
    ../../Source/Core/BlackmagicDeckLink.cpp \
    ../../Source/Core/BlackmagicDeckLink_Glue.cpp \
    ../../Source/Core/FFmpeg_Glue.cpp \
    ../../Source/Core/VideoCore.cpp \
    ../../Source/Core/VideoStats.cpp \
    ../../Source/Core/Timecode.cpp \
    ../../Source/GUI/blackmagicdecklink_userinput.cpp \
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
    ../../Source/GUI/Plot.cpp \
    ../../Source/GUI/Plots.cpp \
    ../../Source/GUI/PlotLegend.cpp \
    ../../Source/GUI/PlotScaleWidget.cpp \
    ../../Source/GUI/preferences.cpp \
    ../../Source/GUI/TinyDisplay.cpp \
    ../../Source/ThirdParty/tinyxml2/tinyxml2.cpp

linux:SOURCES += "../../../Blackmagic DeckLink SDK/Linux/include/DeckLinkAPIDispatch.cpp"
macx:SOURCES += "../../../Blackmagic DeckLink SDK/Mac/include/DeckLinkAPIDispatch.cpp"

win32 {
    INCLUDEPATH += $$[QT_INSTALL_PREFIX]/../src/qtbase/src/3rdparty/zlib
}

FORMS += \
    ../../Source/GUI/mainwindow.ui \
    ../../Source/GUI/preferences.ui \
    ../../Source/GUI/blackmagicdecklink_userinput.ui

RESOURCES += \
    ../../Source/Resource/Resources.qrc

include( $${QWT_ROOT}/qwtconfig.pri )
!win32 {
    include( $${QWT_ROOT}/qwtbuild.pri )
}
include( $${QWT_ROOT}/qwtfunctions.pri )

INCLUDEPATH += $$PWD/../../Source
INCLUDEPATH += $$PWD/../../Source/ThirdParty/tinyxml2
INCLUDEPATH += $$QWT_ROOT/src
INCLUDEPATH += $$PWD/../../../ffmpeg
INCLUDEPATH += "$$PWD/../../../Blackmagic DeckLink SDK"

!win32 {
    LIBS      += -L$${QWT_ROOT}/lib -lqwt
}

win32 {
    CONFIG(debug, debug|release) {
        LIBS += -L$${QWT_ROOT}/lib -lqwtd
    } else:CONFIG(release, debug|release) {
        LIBS += -L$${QWT_ROOT}/lib -lqwt
    }
}

!win32 {
    LIBS += -lz
}

LIBS      += -L$${PWD}/../../../ffmpeg/libavdevice -lavdevice \
             -L$${PWD}/../../../ffmpeg/libavcodec -lavcodec \
             -L$${PWD}/../../../ffmpeg/libavfilter -lavfilter \
             -L$${PWD}/../../../ffmpeg/libavformat -lavformat \
             -L$${PWD}/../../../ffmpeg/libpostproc -lpostproc \
             -L$${PWD}/../../../ffmpeg/libswresample -lswresample \
             -L$${PWD}/../../../ffmpeg/libswscale -lswscale \
             -L$${PWD}/../../../ffmpeg/libavcodec -lavcodec \
             -L$${PWD}/../../../ffmpeg/libavutil -lavutil

macx {
    LIBS      += -lbz2
}

linux {
    LIBS      += -ldl -lrt
}

macx:ICON = ../../Source/Resource/Logo.icns
macx:QMAKE_LFLAGS += -framework CoreFoundation -framework CoreVideo -framework VideoDecodeAcceleration
macx:LIBS += -liconv
