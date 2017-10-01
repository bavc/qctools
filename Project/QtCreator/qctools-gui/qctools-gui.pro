QT       += core gui network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = QCTools
TEMPLATE = app

CONFIG += c++11 qt no_keywords

include(../brew.pri)
message("PWD = " $$PWD)

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

HEADERS += \
    $$SOURCES_PATH/GUI/BigDisplay.h \
    $$SOURCES_PATH/GUI/Control.h \
    $$SOURCES_PATH/GUI/FilesList.h \
    $$SOURCES_PATH/GUI/Help.h \
    $$SOURCES_PATH/GUI/Info.h \
    $$SOURCES_PATH/GUI/mainwindow.h \
    $$SOURCES_PATH/GUI/preferences.h \
    $$SOURCES_PATH/GUI/Comments.h \
    $$SOURCES_PATH/GUI/CommentsEditor.h \
    $$SOURCES_PATH/GUI/Plot.h \
    $$SOURCES_PATH/GUI/Plots.h \
    $$SOURCES_PATH/GUI/PlotLegend.h \
    $$SOURCES_PATH/GUI/PlotScaleWidget.h \
    $$SOURCES_PATH/GUI/TinyDisplay.h \
    $$SOURCES_PATH/GUI/SelectionArea.h \
    $$SOURCES_PATH/GUI/Imagelabel.h \
    $$SOURCES_PATH/GUI/config.h \
    $$SOURCES_PATH/GUI/draggablechildrenbehaviour.h \
    $$SOURCES_PATH/ThirdParty/cqmarkdown/CMarkdown.h \
    $$SOURCES_PATH/GUI/booleanchartconditioneditor.h

SOURCES += \
    $$SOURCES_PATH/GUI/BigDisplay.cpp \
    $$SOURCES_PATH/GUI/Control.cpp \
    $$SOURCES_PATH/GUI/FilesList.cpp \
    $$SOURCES_PATH/GUI/Help.cpp \
    $$SOURCES_PATH/GUI/Info.cpp \
    $$SOURCES_PATH/GUI/main.cpp \
    $$SOURCES_PATH/GUI/mainwindow.cpp \
    $$SOURCES_PATH/GUI/mainwindow_Callbacks.cpp \
    $$SOURCES_PATH/GUI/mainwindow_More.cpp \
    $$SOURCES_PATH/GUI/mainwindow_Ui.cpp \
    $$SOURCES_PATH/GUI/Comments.cpp \
    $$SOURCES_PATH/GUI/CommentsEditor.cpp \
    $$SOURCES_PATH/GUI/Plot.cpp \
    $$SOURCES_PATH/GUI/Plots.cpp \
    $$SOURCES_PATH/GUI/PlotLegend.cpp \
    $$SOURCES_PATH/GUI/PlotScaleWidget.cpp \
    $$SOURCES_PATH/GUI/preferences.cpp \
    $$SOURCES_PATH/GUI/TinyDisplay.cpp \
    $$SOURCES_PATH/GUI/SelectionArea.cpp \
    $$SOURCES_PATH/GUI/Imagelabel.cpp \
    $$SOURCES_PATH/GUI/config.cpp \
    $$SOURCES_PATH/GUI/draggablechildrenbehaviour.cpp \
    $$SOURCES_PATH/ThirdParty/cqmarkdown/CMarkdown.cpp \
    $$SOURCES_PATH/GUI/booleanchartconditioneditor.cpp

win32 {
    INCLUDEPATH += $$[QT_INSTALL_PREFIX]/../src/qtbase/src/3rdparty/zlib
}

FORMS += \
    $$SOURCES_PATH/GUI/mainwindow.ui \
    $$SOURCES_PATH/GUI/preferences.ui \
    $$SOURCES_PATH/GUI/CommentsEditor.ui \
    $$SOURCES_PATH/GUI/blackmagicdecklink_userinput.ui \
    $$SOURCES_PATH/GUI/imagelabel.ui \
    $$SOURCES_PATH/GUI/booleanchartconditioneditor.ui

RESOURCES += \
    $$SOURCES_PATH/Resource/Resources.qrc

help_images_dir="$$SOURCES_PATH/../docs/media"
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
    QWT_ROOT = $$absolute_path($$THIRD_PARTY_PATH/qwt)
    message("use external qwt: QWT_ROOT = " $$QWT_ROOT)

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

INCLUDEPATH += $$SOURCES_PATH
INCLUDEPATH += $$SOURCES_PATH/ThirdParty/cqmarkdown
include(../ffmpeg.pri)
include(../blackmagic.pri)

equals(USE_BLACKMAGIC, true) {

    HEADERS += \
        $$SOURCES_PATH/GUI/blackmagicdecklink_userinput.h

    SOURCES += \
        $$SOURCES_PATH/GUI/blackmagicdecklink_userinput.cpp
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

macx:ICON = $$SOURCES_PATH/Resource/Logo.icns
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
