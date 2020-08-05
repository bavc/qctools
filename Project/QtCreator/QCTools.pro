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

oldConf = $$cat($$QTAV/.qmake.conf.backup, lines)
isEmpty(oldConf) {
    oldConf = $$cat($$QTAV/.qmake.conf, lines)
    message('writting backup of original .qmake.conf')
    write_file($$QTAV/.qmake.conf.backup, oldConf)
} else {
    message('reading backup of original .qmake.conf.backup')
}

message('oldConf: ' $$oldConf)
write_file($$QTAV/.qmake.conf, oldConf)

include(brew.pri)
include(ffmpeg.pri)

contains(DEFINES, USE_BREW) {
    message('using ffmpeg from brew via PKGCONFIG')

    pkgConfig = "PKGCONFIG += libavdevice libavcodec libavfilter libavformat libpostproc libswresample libswscale libavcodec libavutil"
    linkPkgConfig = "CONFIG += link_pkgconfig"

    message('pkgConfig: ' $$pkgConfig)
    message('linkPkgConfig: ' $$linkPkgConfig)

    write_file($$QTAV/.qmake.conf, pkgConfig, append)
    write_file($$QTAV/.qmake.conf, linkPkgConfig, append)
} else {
    ffmpegIncludes = "INCLUDEPATH+=$$FFMPEG_INCLUDES"
    ffmpegLibs = "LIBS+=$$FFMPEG_LIBS"

    message('ffmpegIncludes: ' $$ffmpegIncludes)
    message('ffmpegLibs: ' $$ffmpegLibs)

    staticffmpeg = "CONFIG += static_ffmpeg"
    message('staticffmpeg: ' $$ffmpegLibs)

    write_file($$QTAV/.qmake.conf, ffmpegIncludes, append)
    write_file($$QTAV/.qmake.conf, ffmpegLibs, append)
    write_file($$QTAV/.qmake.conf, staticffmpeg, append)

    # to fix building QtAV with the latest ffmpeg
    limitMacros = "DEFINES += __STDC_LIMIT_MACROS"
    write_file($$QTAV/.qmake.conf, limitMacros, append)
}

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
}

qtav = "QTAV=$$QTAV"
write_file(qctools-gui/.qmake.conf, qtav)

#write_file(qctools-cli/.qmake.conf, ffmpegConfig)
#write_file(qctools-gui/.qmake.conf, ffmpegConfig)
#write_file(qctools-lib/.qmake.conf, ffmpegConfig)

SUBDIRS = \
        qctools-qtav \
        qctools-lib \
        qctools-cli \
        qctools-gui

qctools-qtav.file = qctools-qtav/QtAV.pro

qctools-lib.subdir = qctools-lib
qctools-cli.subdir = qctools-cli
qctools-gui.subdir = qctools-gui

qctools-cli.depends = qctools-lib
qctools-gui.depends = qctools-qtav qctools-lib
