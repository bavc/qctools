TEMPLATE = subdirs

QTAV = $$(QTAV)
isEmpty(QTAV) {
#    QTAV=$$absolute_path(../../../QtAV) // the same level of folder as qctools
    QTAV=$$absolute_path(qctools-qtav)
} else {
    QTAV=$$absolute_path($$QTAV)
}

message('QTAV: ' $$QTAV)
message('QMAKE_COPY: ' $$QMAKE_COPY)
message('QMAKE_COPY_FILE: ' $$QMAKE_COPY_FILE)
message('QMAKE_MKDIR_CMD: ' $$QMAKE_MKDIR_CMD)

FFMPEG = $$(FFMPEG)
isEmpty(FFMPEG) {
    FFMPEG=$$absolute_path(../../../ffmpeg)
}
message('FFMPEG: ' $$FFMPEG)

exists($$FFMPEG/include) {
    FFMPEG_INCLUDE=$$absolute_path($$FFMPEG/include)
} else {
    FFMPEG_INCLUDE=$$FFMPEG
}

exists($$FFMPEG/lib) {
    FFMPEG_AVDEVICE=$$absolute_path($$FFMPEG/lib)
    FFMPEG_AVCODEC=$$absolute_path($$FFMPEG/lib)
    FFMPEG_AVFILTER=$$absolute_path($$FFMPEG/lib)
    FFMPEG_AVFORMAT=$$absolute_path($$FFMPEG/lib)
    FFMPEG_POSTPROC=$$absolute_path($$FFMPEG/lib)
    FFMPEG_SWRESAMPLE=$$absolute_path($$FFMPEG/lib)
    FFMPEG_SWSCALE=$$absolute_path($$FFMPEG/lib)
    FFMPEG_AVUTIL=$$absolute_path($$FFMPEG/lib)

    win32: {
        FFMPEG_LIBS += \
                     -L$$absolute_path($$FFMPEG/lib)
    } else {
        FFMPEG_LIBS += \
                     -L$$absolute_path($$FFMPEG/lib) -lavdevice -lavcodec -lavfilter -lavformat -lpostproc -lswresample -lswscale -lavutil
    }

} else {
    FFMPEG_AVDEVICE=$$absolute_path($$FFMPEG/libavdevice)
    FFMPEG_AVCODEC=$$absolute_path($$FFMPEG/libavcodec)
    FFMPEG_AVFILTER=$$absolute_path($$FFMPEG/libavfilter)
    FFMPEG_AVFORMAT=$$absolute_path($$FFMPEG/libavformat)
    FFMPEG_POSTPROC=$$absolute_path($$FFMPEG/libpostproc)
    FFMPEG_SWRESAMPLE=$$absolute_path($$FFMPEG/libswresample)
    FFMPEG_SWSCALE=$$absolute_path($$FFMPEG/libswscale)
    FFMPEG_AVUTIL=$$absolute_path($$FFMPEG/libavutil)

    win32: {
        FFMPEG_LIBS += \
                     -L$$FFMPEG_AVDEVICE \
                     -L$$FFMPEG_AVCODEC \
                     -L$$FFMPEG_AVFILTER \
                     -L$$FFMPEG_AVFORMAT \
                     -L$$FFMPEG_POSTPROC \
                     -L$$FFMPEG_SWRESAMPLE \
                     -L$$FFMPEG_SWSCALE \
                     -L$$FFMPEG_AVUTIL

    } else {
        FFMPEG_LIBS += \
                     -L$$FFMPEG_AVDEVICE -lavdevice \
                     -L$$FFMPEG_AVCODEC -lavcodec \
                     -L$$FFMPEG_AVFILTER -lavfilter \
                     -L$$FFMPEG_AVFORMAT -lavformat \
                     -L$$FFMPEG_POSTPROC -lpostproc \
                     -L$$FFMPEG_SWRESAMPLE -lswresample \
                     -L$$FFMPEG_SWSCALE -lswscale \
                     -L$$FFMPEG_AVUTIL -lavutil
    }
}

FFMPEG_INCLUDES += $$FFMPEG_INCLUDE

ffmpegIncludes = "INCLUDEPATH+=$$FFMPEG_INCLUDES"
ffmpegLibs = "LIBS+=$$FFMPEG_LIBS"

oldConf = $$cat($$QTAV/.qmake.conf.backup, lines)
isEmpty(oldConf) {
    oldConf = $$cat($$QTAV/.qmake.conf, lines)
    message('writting backup of original .qmake.conf')
    write_file($$QTAV/.qmake.conf.backup, oldConf)
} else {
    message('reading backup of original .qmake.conf.backup')
}

message('oldConf: ' $$oldConf)
message('ffmpegIncludes: ' $$ffmpegIncludes)
message('ffmpegLibs: ' $$ffmpegLibs)

write_file($$QTAV/.qmake.conf, oldConf)
write_file($$QTAV/.qmake.conf, ffmpegIncludes, append)
write_file($$QTAV/.qmake.conf, ffmpegLibs, append)

linux: {
fpic = "QMAKE_CXXFLAGS += -fPIC"
write_file($$QTAV/.qmake.conf, fpic, append)
}

noExamples = CONFIG*=no-examples
write_file($$QTAV/.qmake.conf, noExamples, append)

noTests = CONFIG*=no-tests
write_file($$QTAV/.qmake.conf, noTests, append)

mac: {
noVideoToolbox = "CONFIG*=no-videotoolbox"
write_file($$QTAV/.qmake.conf, noVideoToolbox, append)
noFramework = "CONFIG*=no-framework"
write_file($$QTAV/.qmake.conf, noFramework, append)
}

#write_file(qctools-cli/.qmake.conf, ffmpegConfig)
#write_file(qctools-gui/.qmake.conf, ffmpegConfig)
#write_file(qctools-lib/.qmake.conf, ffmpegConfig)

SUBDIRS = \
        qctools-qtav \
        qctools-lib \
        qctools-cli \
        qctools-gui

qctools-qtav.file = qctools-qtav/qtav.pro

qctools-lib.subdir = qctools-lib
qctools-cli.subdir = qctools-cli
qctools-gui.subdir = qctools-gui

qctools-cli.depends = qctools-lib
qctools-gui.depends = qctools-qtav qctools-lib
