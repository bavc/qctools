USE_BREW = $$(USE_BREW)

macx:!isEmpty(USE_BREW):equals(USE_BREW, true) {
    message("use qwt from brew")
    CONFIG += qwt release

    QMAKE_TARGET_BUNDLE_PREFIX = org.bavc
    QT_CONFIG -= no-pkg-config

    include ( $$system(brew --prefix qwt)/features/qwt.prf )

    CONFIG += link_pkgconfig
} else {

    QWT_ROOT = $$(QWT_ROOT)
    isEmpty(QWT_ROOT) {
        QWT_ROOT = $$absolute_path($$PWD/../../../qwt)
    }

    message("use external qwt: QWT_ROOT = " $$QWT_ROOT)

    include( $${QWT_ROOT}/qwtconfig.pri )
    !win32 {
        include( $${QWT_ROOT}/qwtbuild.pri )
    }
    include( $${QWT_ROOT}/qwtfunctions.pri )

    macx {
        macx:LIBS       += -F$${QWT_ROOT}/lib -framework qwt
    }

    win32-msvc* {
        DEFINES += QWT_DLL
    }

    !macx: {
        win32:CONFIG(release, debug|release): LIBS += -L$${QWT_ROOT}/lib -lqwt
        else:win32:CONFIG(debug, debug|release): LIBS += -L$${QWT_ROOT}/lib -lqwtd
        else: LIBS += -L$${QWT_ROOT}/lib -lqwt
    }

    INCLUDEPATH += $$QWT_ROOT/src

    # copy qwt
    if(equals(MAKEFILE_GENERATOR, MSVC.NET)|equals(MAKEFILE_GENERATOR, MSBUILD)) {
      TRY_COPY = $$QMAKE_COPY
    } else {
      TRY_COPY = -$$QMAKE_COPY #makefile. or -\$\(COPY_FILE\)
    }

    macx {
        qwtlibs.pattern = $$absolute_path($${QWT_ROOT}/lib)
        qwtlibs.files = $$files($$qwtlibs.pattern)
        qwtlibs.path = $$absolute_path($$OUT_PWD$${BUILD_DIR}/$${TARGET}.app/Contents/Frameworks)
        qwtlibs.commands += $$escape_expand(\\n\\t)rm -rf $$shell_path($$qwtlibs.path)

        for(file, qwtlibs.files) {
            qwtlibs.commands += $$escape_expand(\\n\\t)$$QMAKE_COPY_DIR $$shell_path($$file) $$shell_path($$qwtlibs.path)
        }

        qwtlibs.commands += $$escape_expand(\\n\\t)install_name_tool -change qwt.framework/Versions/6/qwt @executable_path/../Frameworks/qwt.framework/Versions/6/qwt $$OUT_PWD$${BUILD_DIR}/$${TARGET}.app/Contents/MacOS/$${TARGET}
    } linux {

        QWTLIBNAME = libqwt

        qwtlibs.pattern = $$absolute_path($${QWT_ROOT}/lib)/$${QWTLIBNAME}.$$QMAKE_EXTENSION_SHLIB*
        #message('qwtlibs.pattern: ' $$qwtlibs.pattern)

        qwtlibs.files = $$files($$qwtlibs.pattern)
        #message('qwtlibs.files: ' $$qwtlibs.files)

        qwtlibs.path = $$absolute_path($$OUT_PWD$${BUILD_DIR})
        #message('qwtlibs.path: ' $$qwtlibs.path)

        for(file, qwtlibs.files) {
            qwtlibs.commands += $$escape_expand(\\n\\t)$$TRY_COPY $$shell_path($$file) $$shell_path($$qwtlibs.path)
        }
        #message('qwtlibs.commands: ' $$qwtlibs.commands)

        # make linker to search for qwt in current directory
        QMAKE_RPATHDIR += .

    } win32 {

        CONFIG(debug, debug|release) {
            BUILD_SUFFIX=d
            BUILD_DIR=/debug
        } else:CONFIG(release, debug|release) {
            BUILD_SUFFIX=
            BUILD_DIR=/release
        }
        QWTLIBNAME = qwt$${BUILD_SUFFIX}

        qwtlibs.pattern = $$absolute_path($${QWT_ROOT}/lib)/$${QWTLIBNAME}.$$QMAKE_EXTENSION_SHLIB
        qwtlibs.files = $$files($$qwtlibs.pattern)
        qwtlibs.path = $$absolute_path($$OUT_PWD$${BUILD_DIR})

        for(file, qwtlibs.files) {
            qwtlibs.commands += $$escape_expand(\\n\\t)$$TRY_COPY $$shell_path($$file) $$shell_path($$qwtlibs.path)
        }
    }

    isEmpty(QMAKE_POST_LINK): QMAKE_POST_LINK = $$qwtlibs.commands
    else: QMAKE_POST_LINK = $${QMAKE_POST_LINK}$$escape_expand(\\n\\t)$$qwtlibs.commands
}
