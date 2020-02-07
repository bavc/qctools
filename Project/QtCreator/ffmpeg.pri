macx:contains(DEFINES, USE_BREW) {
    message("use ffmpeg from brew")

    PKGCONFIG += libavdevice libavcodec libavfilter libavformat libpostproc
    PKGCONFIG += libswresample libswscale libavcodec libavutil

    CONFIG += link_pkgconfig

} else {
    FFMPEG_PATH = $$absolute_path($$PWD/../../../ffmpeg)
    message("add external ffmpeg " $$FFMPEG_PATH )

    INCLUDEPATH += $$FFMPEG_PATH

    unix {
            LIBS      += -L$$FFMPEG_PATH/libavdevice -lavdevice \
                         -L$$FFMPEG_PATH/libavcodec -lavcodec \
                         -L$$FFMPEG_PATH/libavfilter -lavfilter \
                         -L$$FFMPEG_PATH/libavformat -lavformat \
                         -L$$FFMPEG_PATH/libpostproc -lpostproc \
                         -L$$FFMPEG_PATH/libswresample -lswresample \
                         -L$$FFMPEG_PATH/libswscale -lswscale \
                         -L$$FFMPEG_PATH/libavcodec -lavcodec \
                         -L$$FFMPEG_PATH/libavutil -lavutil
    }

   win32 {

            LIBS      += -L$$FFMPEG_PATH/lib -lavdevice \
                         -L$$FFMPEG_PATH/lib -lavcodec \
                         -L$$FFMPEG_PATH/lib -lavfilter \
                         -L$$FFMPEG_PATH/lib -lavformat \
                         -L$$FFMPEG_PATH/lib -lpostproc \
                         -L$$FFMPEG_PATH/lib -lswresample \
                         -L$$FFMPEG_PATH/lib -lswscale \
                         -L$$FFMPEG_PATH/lib -lavcodec \
                         -L$$FFMPEG_PATH/lib -lavutil

   }

}
