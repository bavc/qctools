win32 {
    ZLIB_INCLUDE_PATH = $$[QT_INSTALL_HEADERS]/QtZlib
    message("qctools-lib: ZLIB_INCLUDE_PATH = " $$ZLIB_INCLUDE_PATH)
    INCLUDEPATH += $$ZLIB_INCLUDE_PATH
}

unix {
    LIBS += -lz
}
