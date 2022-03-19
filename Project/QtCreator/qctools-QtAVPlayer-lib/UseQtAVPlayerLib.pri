INCLUDEPATH += $$absolute_path($$QTAVPLAYER_SRC/src) $$absolute_path($$QTAVPLAYER_SRC/src/QtAVPlayer)
message('UseQtAVPlayerLib.pri INCLUDEPATH: ' $$INCLUDEPATH)

win32: {
    CONFIG(debug, debug|release) {
        BUILD_SUFFIX=d
        BUILD_DIR=/debug
    } else:CONFIG(release, debug|release) {
        BUILD_SUFFIX=
        BUILD_DIR=/release
    }

    win32-g++: {
        BUILD_SUFFIX=
    }

}
!win32: {
    QTLIBPREFIX = lib
}

QTAVPLAYER_BUILD_DIR = $$QTAVPLAYER_LIB/src/QtAVPlayer$${BUILD_DIR}
message('QTAVPLAYER_BUILD_DIR: ' $$QTAVPLAYER_BUILD_DIR)

LIBS += -L$$QTAVPLAYER_BUILD_DIR -lQtAVPlayer
message('UseQtAVPlayerLib.pri LIBS: ' $$LIBS)

# deploy
if(equals(MAKEFILE_GENERATOR, MSVC.NET)|equals(MAKEFILE_GENERATOR, MSBUILD)) {
  TRY_COPY = $$QMAKE_COPY
} else {
  TRY_COPY = -$$QMAKE_COPY #makefile. or -\$\(COPY_FILE\)
}

QTAVLIBNAME = QtAVPlayer
mac: {
  qtavplayerlibs.pattern = $$QTAVPLAYER_BUILD_DIR/$$QTLIBPREFIX$${QTAVLIBNAME}*.$$QMAKE_EXTENSION_SHLIB*
  message('qtavplayerlibs.pattern: ' $$qtavplayerlibs.pattern)

  qtavplayerlibs.files = $$files($$qtavplayerlibs.pattern)
  message('qtavplayerlibs.files: ' $$qtavplayerlibs.files)

  qtavplayerlibs.path = $$absolute_path($$OUT_PWD$${BUILD_DIR}/$${TARGET}.app/Contents/Libraries)
  message('qtavplayerlibs.path: ' $$qtavplayerlibs.path)

  qtavplayerlibs.commands += $$escape_expand(\\n\\t)rm -rf $$shell_path($$qtavplayerlibs.path)
  qtavplayerlibs.commands += $$escape_expand(\\n\\t)$$QMAKE_MKDIR $$shell_path($$qtavplayerlibs.path)

  for(item, qtavplayerlibs.files) {
    message('adding file ' $$item ' to copy list')
    qtavplayerlibs.commands += $$escape_expand(\\n\\t)$$QMAKE_COPY_DIR -L $$shell_path($$item) $$shell_path($$qtavplayerlibs.path)/
  }

  qtavplayerlibs.commands += $$escape_expand(\\n\\t)install_name_tool -change libQtAVPlayer.1.dylib @executable_path/../Libraries/libQtAVPlayer.1.dylib $$OUT_PWD$${BUILD_DIR}/$${TARGET}.app/Contents/MacOS/$${TARGET}

} else {
  qtavplayerlibs.pattern = $$QTAVPLAYER_BUILD_DIR/$$QTLIBPREFIX$${QTAVLIBNAME}.$$QMAKE_EXTENSION_SHLIB*
  message('qtavplayerlibs.pattern: ' $$qtavplayerlibs.pattern)

  qtavplayerlibs.files = $$files($$qtavplayerlibs.pattern)
  message('qtavplayerlibs.files: ' $$qtavplayerlibs.files)

  qtavplayerlibs.path = $$absolute_path($$OUT_PWD$${BUILD_DIR})
  message('qtavplayerlibs.path: ' $$qtavplayerlibs.path)

  for(item, qtavplayerlibs.files) {
    message('adding file ' $$item ' to copy list')
    qtavplayerlibs.commands += $$escape_expand(\\n\\t)$$TRY_COPY $$shell_path($$item) $$shell_path($$qtavplayerlibs.path)
  }
}

message('qtavplayerlibs.commands: ' $$qtavplayerlibs.commands)

isEmpty(QMAKE_POST_LINK): QMAKE_POST_LINK = $$qtavplayerlibs.commands
else: QMAKE_POST_LINK = $${QMAKE_POST_LINK}$$escape_expand(\\n\\t)$$qtavplayerlibs.commands

message('QMAKE_POST_LINK: ' $$QMAKE_POST_LINK)
