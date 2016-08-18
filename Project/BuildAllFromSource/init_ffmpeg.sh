#! /bin/bash

echo "PWD: " + $PWD

_install_yasm(){
    wget http://www.tortall.net/projects/yasm/releases/yasm-1.3.0.tar.gz
    tar -zxvf yasm-1.3.0.tar.gz
    rm yasm-1.3.0.tar.gz
    mv yasm-1.3.0 yasm
}

if [ ! -d ffmpeg ] ; then
    git clone git://source.ffmpeg.org/ffmpeg.git ffmpeg
fi

    cd ffmpeg
    FFMPEG_CONFIGURE_OPTS=(--enable-gpl --enable-version3 --disable-securetransport --disable-videotoolbox --disable-shared --enable-static --disable-swscale-alpha --disable-doc --disable-ffplay --disable-ffprobe --disable-ffserver --disable-debug --disable-lzma)
    if sw_vers >/dev/null 2>&1 ; then
        FFMPEG_CONFIGURE_OPTS+=(--extra-cflags="-mmacosx-version-min=10.5" --extra-ldflags="-mmacosx-version-min=10.5")
    fi
    chmod u+x configure
    chmod u+x version.sh
    if yasm --version >/dev/null 2>&1 ; then
        ./configure "${FFMPEG_CONFIGURE_OPTS[@]}"
        if [ "$?" -ne 0 ] ; then #on some distro, yasm version is too old
            cd "$INSTALL_DIR"
            if [ ! -d yasm ] ; then
                _install_yasm
            fi
            cd yasm/
            ./configure --prefix=`pwd`/usr
            make
            cd "${INSTALL_DIR}/ffmpeg"
            FFMPEG_CONFIGURE_OPTS+=(--yasmexe=../yasm/usr/bin/yasm)
            ./configure "${FFMPEG_CONFIGURE_OPTS[@]}"
        fi
    else
        cd "$INSTALL_DIR"
        if [ ! -d yasm ] ; then
            _install_yasm
        fi
        cd yasm/
        ./configure --prefix=`pwd`/usr
        make
        make install
        cd "${INSTALL_DIR}/ffmpeg"
        FFMPEG_CONFIGURE_OPTS+=(--yasmexe=../yasm/usr/bin/yasm)
        ./configure "${FFMPEG_CONFIGURE_OPTS[@]}"
    fi
    make
    cd "$INSTALL_DIR"
