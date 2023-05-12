win32 {
    message("QT_INSTALL_PREFIX = " $$[QT_INSTALL_PREFIX])
    message("3rdparty path = " $$absolute_path($$[QT_INSTALL_PREFIX]/../src/qtbase/src/3rdparty))

    exists($$[QT_INSTALL_PREFIX]/../src/qtbase/src/3rdparty) {
        greaterThan(QT_MINOR_VERSION, 8) || greaterThan(QT_MAJOR_VERSION, 5): {
            ZLIB_INCLUDE_PATH = $$absolute_path($$[QT_INSTALL_PREFIX]/../src/qtbase/src/3rdparty/zlib/src)
            greaterThan(QT_MINOR_VERSION, 13): {
                zlibPri = $$absolute_path($$[QT_INSTALL_PREFIX]/../src/qtbase/src/3rdparty/zlib.pri)
                include($${zlibPri})
            } else {
                ZLIB_INCLUDE_PATH = $$absolute_path($$[QT_INSTALL_PREFIX]/../src/qtbase/src/3rdparty/zlib/src)
            }
        } else {
            ZLIB_INCLUDE_PATH = $$absolute_path($$[QT_INSTALL_PREFIX]/../src/qtbase/src/3rdparty/zlib)
        }

        message("use zlib from Qt sources: " $${ZLIB_INCLUDE_PATH})
    } else {
        ZLIB_INCLUDE_PATH = $${THIRD_PARTY_PATH}/zlib/include

        win32-msvc* {
            LIBS += $${THIRD_PARTY_PATH}/zlib/lib/zlibstatic.lib
        }

        win32-g++* {
            LIBS += $${THIRD_PARTY_PATH}/zlib/lib/libzlibstatic.a
        }
    }

    message("qctools: ZLIB_INCLUDE_PATH = " $$ZLIB_INCLUDE_PATH)
    INCLUDEPATH += $$ZLIB_INCLUDE_PATH
}

unix {
    LIBS += -lz
}
