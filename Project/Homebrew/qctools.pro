QT += core gui widgets svg printsupport

TARGET = QCTools
TEMPLATE = app

QT_CONFIG -= no-pkg-config

include ( $$system(brew --prefix qwt)/features/qwt.prf )

CONFIG += qt qwt release no_keywords

PKGCONFIG += libavdevice libavcodec libavfilter libavformat libpostproc
PKGCONFIG += libswresample libswscale libavcodec libavutil freetype2

CONFIG += link_pkgconfig

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

FORMS += \
    ../../Source/GUI/mainwindow.ui \
    ../../Source/GUI/preferences.ui \
    ../../Source/GUI/blackmagicdecklink_userinput.ui

RESOURCES += \
    ../../Source/Resource/Resources.qrc

INCLUDEPATH += $$PWD/../../Source
INCLUDEPATH += $$PWD/../../Source/ThirdParty/tinyxml2

LIBS      += -lz
LIBS      += -lbz2

!macx:LIBS      += -ldl -lrt

macx:ICON = ../../Source/Resource/Logo.icns
macx:QMAKE_LFLAGS += -framework CoreFoundation -framework CoreVideo -framework VideoDecodeAcceleration
macx:LIBS += -liconv
