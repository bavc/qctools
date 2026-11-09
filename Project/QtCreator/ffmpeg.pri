message('entering ffmpeg.pri')

USE_BREW = $$(USE_BREW)
isEmpty(FFMPEG_USE_SYSTEM): FFMPEG_USE_SYSTEM = $$(FFMPEG_USE_SYSTEM)
isEmpty(FFMPEG_LIBRARY_SUFFIX): FFMPEG_LIBRARY_SUFFIX = $$(FFMPEG_LIBRARY_SUFFIX)

FFMPEG_USE_PKGCONFIG = false
macx:!isEmpty(USE_BREW):equals(USE_BREW, true): FFMPEG_USE_PKGCONFIG = true
!isEmpty(FFMPEG_USE_SYSTEM):equals(FFMPEG_USE_SYSTEM, true): FFMPEG_USE_PKGCONFIG = true

contains(FFMPEG_USE_PKGCONFIG, true) {
    message("use ffmpeg from pkg-config")

    PKGCONFIG += libavdevice$${FFMPEG_LIBRARY_SUFFIX} libavcodec$${FFMPEG_LIBRARY_SUFFIX}
    PKGCONFIG += libavfilter$${FFMPEG_LIBRARY_SUFFIX} libavformat$${FFMPEG_LIBRARY_SUFFIX}
    PKGCONFIG += libswresample$${FFMPEG_LIBRARY_SUFFIX} libswscale$${FFMPEG_LIBRARY_SUFFIX}
    PKGCONFIG += libavcodec$${FFMPEG_LIBRARY_SUFFIX} libavutil$${FFMPEG_LIBRARY_SUFFIX}

    CONFIG += link_pkgconfig

} else {
    FFMPEG = $$(FFMPEG)
    isEmpty(FFMPEG) {
        FFMPEG=$$absolute_path(../../../ffmpeg)
        message('FFMPEG: ' $$FFMPEG)
    } else {
        message('specified FFMPEG: ' $$FFMPEG)
    }

    exists($$FFMPEG/include) {
        FFMPEG_INCLUDES=$$absolute_path($$FFMPEG/include)
    } else {
        FFMPEG_INCLUDES=$$FFMPEG
    }

    win32:!contains(STATIC, yes|1):exists($$FFMPEG/bin/avcodec$${FFMPEG_LIBRARY_SUFFIX}.lib) {
        FFMPEG_LIBS += -L$$absolute_path($$FFMPEG/bin) \
                       -lavdevice$${FFMPEG_LIBRARY_SUFFIX} \
                       -lavcodec$${FFMPEG_LIBRARY_SUFFIX} \
                       -lavfilter$${FFMPEG_LIBRARY_SUFFIX} \
                       -lavformat$${FFMPEG_LIBRARY_SUFFIX} \
                       -lswresample$${FFMPEG_LIBRARY_SUFFIX} \
                       -lswscale$${FFMPEG_LIBRARY_SUFFIX} \
                       -lavutil$${FFMPEG_LIBRARY_SUFFIX}
    } else:exists($$FFMPEG/lib/avcodec$${FFMPEG_LIBRARY_SUFFIX}.lib) {
        FFMPEG_LIBS += -L$$absolute_path($$FFMPEG/lib) \
                        -lavdevice$${FFMPEG_LIBRARY_SUFFIX} \
                        -lavfilter$${FFMPEG_LIBRARY_SUFFIX} \
                        -lavformat$${FFMPEG_LIBRARY_SUFFIX} \
                        -lavcodec$${FFMPEG_LIBRARY_SUFFIX} \
                        -lswresample$${FFMPEG_LIBRARY_SUFFIX} \
                        -lswscale$${FFMPEG_LIBRARY_SUFFIX} \
                        -lavutil$${FFMPEG_LIBRARY_SUFFIX}
        win32 {
            FFMPEG_LIBS += -lUser32 -lOle32 -lSecur32 -lbcrypt -lwsock32 -lws2_32 -lVfw32 -lmfplat -lmfuuid -lstrmiids
        }
    } else:macx:exists($$FFMPEG/lib/libavcodec$${FFMPEG_LIBRARY_SUFFIX}.dylib) {
        FFMPEG_LIBS += -L$$absolute_path($$FFMPEG/lib) \
                     -lavdevice$${FFMPEG_LIBRARY_SUFFIX} \
                     -lavfilter$${FFMPEG_LIBRARY_SUFFIX} \
                     -lavformat$${FFMPEG_LIBRARY_SUFFIX} \
                     -lavcodec$${FFMPEG_LIBRARY_SUFFIX} \
                     -lswresample$${FFMPEG_LIBRARY_SUFFIX} \
                     -lswscale$${FFMPEG_LIBRARY_SUFFIX} \
                     -lavutil$${FFMPEG_LIBRARY_SUFFIX}
    } else {
        FFMPEG_AVDEVICE=$$absolute_path($$FFMPEG/libavdevice)
        FFMPEG_AVCODEC=$$absolute_path($$FFMPEG/libavcodec)
        FFMPEG_AVFILTER=$$absolute_path($$FFMPEG/libavfilter)
        FFMPEG_AVFORMAT=$$absolute_path($$FFMPEG/libavformat)
        FFMPEG_SWRESAMPLE=$$absolute_path($$FFMPEG/libswresample)
        FFMPEG_SWSCALE=$$absolute_path($$FFMPEG/libswscale)
        FFMPEG_AVUTIL=$$absolute_path($$FFMPEG/libavutil)

        FFMPEG_LIBS += \
                     -L$$FFMPEG_AVDEVICE -lavdevice$${FFMPEG_LIBRARY_SUFFIX} \
                     -L$$FFMPEG_AVFILTER -lavfilter$${FFMPEG_LIBRARY_SUFFIX} \
                     -L$$FFMPEG_AVFORMAT -lavformat$${FFMPEG_LIBRARY_SUFFIX} \
                     -L$$FFMPEG_AVCODEC -lavcodec$${FFMPEG_LIBRARY_SUFFIX} \
                     -L$$FFMPEG_SWRESAMPLE -lswresample$${FFMPEG_LIBRARY_SUFFIX} \
                     -L$$FFMPEG_SWSCALE -lswscale$${FFMPEG_LIBRARY_SUFFIX} \
                     -L$$FFMPEG_AVUTIL -lavutil$${FFMPEG_LIBRARY_SUFFIX}
    }

    contains(STATIC, yes|1) {
        FFMPEG_LIBS += -L$$absolute_path($$FFMPEG/../output/lib) -lharfbuzz
        FFMPEG_LIBS += -L$$absolute_path($$FFMPEG/../output/lib) -lfreetype
    }

    INCLUDEPATH += $$FFMPEG_INCLUDES
    LIBS += $$FFMPEG_LIBS

    message('ffmpeg.pri INCLUDEPATH: ' $$INCLUDEPATH)
    message('ffmpeg.pri LIBS: ' $$LIBS)
}

message('leaving ffmpeg.pri')
