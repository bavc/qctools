QT       += core gui network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = QCTools
TEMPLATE = app

CONFIG += c++11 qt no_keywords

USE_BREW = $$(QCTOOLS_USE_BREW)

QCTOOLS_USE_BREW_NOT_EMPTY = false

!isEmpty(USE_BREW) {
    message("USE_BREW not empty")
    QCTOOLS_USE_BREW_NOT_EMPTY = true
}

QCTOOLS_USE_BREW_EQUALS_TRUE = false

equals(USE_BREW, true) {
    message("USE_BREW equals true")
    QCTOOLS_USE_BREW_EQUALS_TRUE = true
}

message("QCTOOLS_USE_BREW_NOT_EMPTY = " $$QCTOOLS_USE_BREW_NOT_EMPTY )
message("QCTOOLS_USE_BREW_EQUALS_TRUE = " $$QCTOOLS_USE_BREW_EQUALS_TRUE )

!isEmpty(USE_BREW):equals(USE_BREW, true) {
    message("DEFINES += USE_BREW")
    DEFINES += USE_BREW
}

macx:contains(DEFINES, USE_BREW) {

    message("use brew")
    CONFIG += qwt release

    QMAKE_TARGET_BUNDLE_PREFIX = org.bavc
    QT_CONFIG -= no-pkg-config

    include ( $$system(brew --prefix qwt-qt5)/features/qwt.prf )

    PKGCONFIG += libavdevice libavcodec libavfilter libavformat libpostproc
    PKGCONFIG += libswresample libswscale libavcodec libavutil

    CONFIG += link_pkgconfig

} else {
    CONFIG += debug_and_release
}

USE_BLACKMAGIC = $$(QCTOOLS_USE_BLACKMAGIC)
equals(USE_BLACKMAGIC, true) {
    message("QCTOOLS_USE_BLACKMAGIC is true, blackmagic integration enabled ")
    DEFINES += BLACKMAGICDECKLINK_YES

    win32 {
        IDL = "../../../Blackmagic DeckLink SDK/Win/include/DeckLinkAPI.idl"
        idl_c.output = ${QMAKE_FILE_IN}.h
        idl_c.input = IDL
        idl_c.commands = $${QMAKE_IDL} ${QMAKE_FILE_IN} $${IDLFLAGS} \
                         /h ${QMAKE_FILE_IN}.h /iid ${QMAKE_FILE_IN}.c
        idl_c.variable_out = SOURCES
        idl_c.name = MIDL
        idl_c.clean = ${QMAKE_FILE_IN}.h ${QMAKE_FILE_IN}.c
        idl_c.CONFIG = no_link target_predeps

        QMAKE_EXTRA_COMPILERS += idl_c

        LIBS += -lOle32
    }

    linux:SOURCES += "../../../Blackmagic DeckLink SDK/Linux/include/DeckLinkAPIDispatch.cpp"
    macx:!contains(DEFINES, USE_BREW) SOURCES += "../../../Blackmagic DeckLink SDK/Mac/include/DeckLinkAPIDispatch.cpp"

    HEADERS += \
        ../../Source/Core/BlackmagicDeckLink.h \
        ../../Source/Core/BlackmagicDeckLink_Glue.h \
        ../../Source/GUI/blackmagicdecklink_userinput.h


    SOURCES += \
        ../../Source/Core/BlackmagicDeckLink.cpp \
        ../../Source/Core/BlackmagicDeckLink_Glue.cpp \
        ../../Source/GUI/blackmagicdecklink_userinput.cpp

} else {
    message("QCTOOLS_USE_BLACKMAGIC is not true, blackmagic integration disabled")
}

QMAKE_CXXFLAGS += -DWITH_SYSTEM_FFMPEG=1

HEADERS += \
    ../../Source/Core/AudioCore.h \
    ../../Source/Core/AudioStats.h \
    ../../Source/Core/CommonStats.h \
    ../../Source/Core/Core.h \
    ../../Source/Core/FFmpeg_Glue.h \
    ../../Source/Core/VideoCore.h \
    ../../Source/Core/VideoStats.h \
    ../../Source/Core/FormatStats.h \
    ../../Source/Core/CommonStreamStats.h \
    ../../Source/Core/AudioStreamStats.h \
    ../../Source/Core/VideoStreamStats.h \
    ../../Source/Core/StreamsStats.h \
    ../../Source/Core/Timecode.h \
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
    ../../Source/GUI/SelectionArea.h \
    ../../Source/ThirdParty/tinyxml2/tinyxml2.h \
    ../../Source/GUI/Imagelabel.h \
    ../../Source/GUI/config.h \
    ../../Source/GUI/draggablechildrenbehaviour.h \
    ../../Source/GUI/SignalServerConnectionChecker.h \
    ../../Source/GUI/SignalServer.h \
    ../../Source/ThirdParty/cqmarkdown/CMarkdown.h

SOURCES += \
    ../../Source/Core/AudioCore.cpp \
    ../../Source/Core/AudioStats.cpp \
    ../../Source/Core/CommonStats.cpp \
    ../../Source/Core/Core.cpp \
    ../../Source/Core/FFmpeg_Glue.cpp \
    ../../Source/Core/VideoCore.cpp \
    ../../Source/Core/VideoStats.cpp \
    ../../Source/Core/FormatStats.cpp \
    ../../Source/Core/CommonStreamStats.cpp \
    ../../Source/Core/AudioStreamStats.cpp \
    ../../Source/Core/VideoStreamStats.cpp \
    ../../Source/Core/StreamsStats.cpp \
    ../../Source/Core/Timecode.cpp \
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
    ../../Source/GUI/SelectionArea.cpp \
    ../../Source/ThirdParty/tinyxml2/tinyxml2.cpp \
    ../../Source/GUI/Imagelabel.cpp \
    ../../Source/GUI/config.cpp \
    ../../Source/GUI/draggablechildrenbehaviour.cpp \
    ../../Source/GUI/SignalServerConnectionChecker.cpp \
    ../../Source/GUI/SignalServer.cpp \
    ../../Source/ThirdParty/cqmarkdown/CMarkdown.cpp

win32 {
    INCLUDEPATH += $$[QT_INSTALL_PREFIX]/../src/qtbase/src/3rdparty/zlib
}

FORMS += \
    ../../Source/GUI/mainwindow.ui \
    ../../Source/GUI/preferences.ui \
    ../../Source/GUI/blackmagicdecklink_userinput.ui \
    ../../Source/GUI/imagelabel.ui

RESOURCES += \
    ../../Source/Resource/Resources.qrc

help_images_dir="$$PWD/../../Docs/media"
help_images.files = $$files($$help_images_dir/*, true)
help_images.prefix = "/Help"
help_images.alias = "./"

DRESOURCES += help_images

# http://www.w3.org/TR/xml/#syntax
defineReplace(xml_escape) {
    1 ~= s,&,&amp;,
    1 ~= s,\',&apos;,
    1 ~= s,\",&quot;,
    1 ~= s,<,&lt;,
    1 ~= s,>,&gt;,
    return($$1)
}

for(resource, DRESOURCES) {

    # Regular case of user qrc file
    contains(resource, ".*\.qrc$"): \
        next()

    # Fallback for stand-alone files/directories
    !defined($${resource}.files, var) {
        !equals(resource, qmake_immediate) {
            !exists($$absolute_path($$resource, $$_PRO_FILE_PWD_)): \
                warning("Failure to find: $$resource")
            qmake_immediate.files += $$resource
        }
        RESOURCES -= $$resource
        next()
    }

    resource_alias = $$eval($${resource}.alias)
    resource_file = $$RCC_DIR/qmake_$${resource}.qrc

    !debug_and_release|build_pass {
        # Collection of files, generate qrc file
        prefix = $$eval($${resource}.prefix)
        isEmpty(prefix): \
            prefix = "/"

        resource_file_content = \
            "<!DOCTYPE RCC><RCC version=\"1.0\">" \
            "<qresource prefix=\"$$xml_escape($$prefix)\">"

        abs_base = $$absolute_path($$eval($${resource}.base), $$_PRO_FILE_PWD_)

        for(file, $${resource}.files) {
            abs_path = $$absolute_path($$file, $$_PRO_FILE_PWD_)

            isEqual(resource_alias, "") {
                alias = $$relative_path($$abs_path, $$abs_base)
            } else {
                alias = $$resource_alias$$basename(file)
            }

            #message("alias: " $$alias)

            resource_file_content += \
                "<file alias=\"$$xml_escape($$alias)\">$$xml_escape($$abs_path)</file>"
        }

        resource_file_content += \
            "</qresource>" \
            "</RCC>"

        !write_file($$OUT_PWD/$$resource_file, resource_file_content): \
            error("Aborting.")
    }

    RESOURCES -= $$resource
    RESOURCES += $$OUT_PWD/$$resource_file
}

macx:contains(DEFINES, USE_BREW) {
    message("use qwt from brew")
} else {
    QWT_ROOT = $${PWD}/../../../qwt

    include( $${QWT_ROOT}/qwtconfig.pri )
    !win32 {
        include( $${QWT_ROOT}/qwtbuild.pri )
    }
    include( $${QWT_ROOT}/qwtfunctions.pri )

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

    INCLUDEPATH += $$QWT_ROOT/src
}
INCLUDEPATH += $$PWD/../../Source
INCLUDEPATH += $$PWD/../../Source/ThirdParty/tinyxml2
INCLUDEPATH += $$PWD/../../Source/ThirdParty/cqmarkdown
include($$PWD/../../Source/ThirdParty/qblowfish/qblowfish.pri)

macx:contains(DEFINES, USE_BREW) {
    message("use ffmpeg from brew")
} else {
    INCLUDEPATH += $$PWD/../../../ffmpeg

    LIBS      += -L$${PWD}/../../../ffmpeg/libavdevice -lavdevice \
                 -L$${PWD}/../../../ffmpeg/libavcodec -lavcodec \
                 -L$${PWD}/../../../ffmpeg/libavfilter -lavfilter \
                 -L$${PWD}/../../../ffmpeg/libavformat -lavformat \
                 -L$${PWD}/../../../ffmpeg/libpostproc -lpostproc \
                 -L$${PWD}/../../../ffmpeg/libswresample -lswresample \
                 -L$${PWD}/../../../ffmpeg/libswscale -lswscale \
                 -L$${PWD}/../../../ffmpeg/libavcodec -lavcodec \
                 -L$${PWD}/../../../ffmpeg/libavutil -lavutil
}

macx:contains(DEFINES, USE_BREW) {
    message("don't use Blackmagic DeckLink SDK for brew build")
} else {
    INCLUDEPATH += "$$PWD/../../../Blackmagic DeckLink SDK"
}

!win32 {
    LIBS += -lz
}

!win32 {
    LIBS      += -lbz2
}

unix {
    LIBS       += -ldl
    !macx:LIBS += -lrt
}

macx:ICON = ../../Source/Resource/Logo.icns
macx:LIBS += -liconv \
	     -framework CoreFoundation \
             -framework Foundation \
             -framework AppKit \
             -framework AudioToolBox \
             -framework QuartzCore \
             -framework CoreGraphics \
             -framework CoreAudio \
             -framework CoreVideo \
             -framework VideoDecodeAcceleration
