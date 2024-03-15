#! /bin/bash

echo "PWD: " + $PWD

_install_yasm(){
    wget -q http://www.tortall.net/projects/yasm/releases/yasm-1.3.0.tar.gz
    tar -zxf yasm-1.3.0.tar.gz
    mv yasm-1.3.0 yasm
}

if [ ! -d ffmpeg ] ; then
    git clone --depth 1 git://source.ffmpeg.org/ffmpeg.git ffmpeg
fi

if [ ! -d freetype ] ; then
    curl -LO https://download.savannah.gnu.org/releases/freetype/freetype-2.10.0.tar.bz2
    tar -xf freetype-2.10.0.tar.bz2
    mv freetype-2.10.0 freetype
fi

if [ ! -d harfbuzz ] ; then
    curl -LO https://github.com/harfbuzz/harfbuzz/releases/download/8.2.2/harfbuzz-8.2.2.tar.xz
    tar -Jxf harfbuzz-8.2.2.tar.xz
    rm harfbuzz-8.2.2.tar.xz
    mv harfbuzz-8.2.2 harfbuzz
fi

cd freetype
chmod u+x configure
if sw_vers >/dev/null 2>&1 ; then
./configure --prefix="$(pwd)/usr" --without-harfbuzz --without-zlib --without-bzip2 --without-png --enable-static --disable-shared CFLAGS=-mmacosx-version-min=10.12 LDFLAGS=-mmacosx-version-min=10.12
else
./configure --prefix="$(pwd)/usr" --without-harfbuzz --without-zlib --without-bzip2 --without-png --enable-static --disable-shared
fi
make
make install
cd ..

cd harfbuzz
mkdir build
cd build
if sw_vers >/dev/null 2>&1 ; then
CFLAGS=-mmacosx-version-min=10.12 LDFLAGS=-mmacosx-version-min=10.12 PKG_CONFIG_PATH=$PWD/../../freetype/usr/lib/pkgconfig meson setup --prefix $(pwd)/../usr --default-library=static -Dglib=disabled -Dgobject=disabled -Dcairo=disabled -Dchafa=disabled -Dicu=disabled -Dgraphite=disabled -Dgraphite2=disabled -Dgdi=disabled -Ddirectwrite=disabled -Dcoretext=disabled -Dwasm=disabled -Dtests=disabled -Dintrospection=disabled -Ddocs=disabled -Ddoc_tests=false -Dutilities=disabled ..
else
PKG_CONFIG_PATH=$PWD/../../freetype/usr/lib/pkgconfig meson setup --prefix $(pwd)/../usr --default-library=static -Dglib=disabled -Dgobject=disabled -Dcairo=disabled -Dchafa=disabled -Dicu=disabled -Dgraphite=disabled -Dgraphite2=disabled -Dgdi=disabled -Ddirectwrite=disabled -Dcoretext=disabled -Dwasm=disabled -Dtests=disabled -Dintrospection=disabled -Ddocs=disabled -Ddoc_tests=false -Dutilities=disabled ..
fi
ninja
ninja install
cd ..
cd ..

cd ffmpeg
FFMPEG_CONFIGURE_OPTS=(--enable-gpl --enable-version3 --disable-autodetect --disable-programs --disable-securetransport --disable-videotoolbox --enable-static --disable-shared --disable-doc --disable-debug --disable-lzma --disable-iconv --enable-pic --prefix="$(pwd)" --enable-libfreetype --enable-libharfbuzz --extra-cflags="-I../freetype/usr/include/freetype2" --extra-cflags="-I../harfbuzz/usr/include/harfbuzz" --extra-libs="../freetype/usr/lib/libfreetype.a" --extra-libs="../harfbuzz/usr/lib/libharfbuzz.a")
if sw_vers >/dev/null 2>&1 ; then
    FFMPEG_CONFIGURE_OPTS+=(--extra-cflags="-mmacosx-version-min=10.12" --extra-ldflags="-mmacosx-version-min=10.12")
fi

chmod u+x configure
chmod u+x version.sh
if yasm --version >/dev/null 2>&1 ; then
    echo "FFMPEG_CONFIGURE_OPTS = ${FFMPEG_CONFIGURE_OPTS[@]}"
    ./configure "${FFMPEG_CONFIGURE_OPTS[@]}"
    if [ "$?" -ne 0 ] ; then #on some distro, yasm version is too old
        cd "$INSTALL_DIR"
        if [ ! -d yasm ] ; then
            _install_yasm
        fi
        cd yasm/
        ./configure --prefix="$(pwd)/usr"
        make
        make install
        cd "${INSTALL_DIR}/ffmpeg"
        FFMPEG_CONFIGURE_OPTS+=(--x86asmexe=../yasm/usr/bin/yasm)
        echo "FFMPEG_CONFIGURE_OPTS = ${FFMPEG_CONFIGURE_OPTS[@]}"
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
    FFMPEG_CONFIGURE_OPTS+=(--x86asmexe=../yasm/usr/bin/yasm)
    echo "FFMPEG_CONFIGURE_OPTS = ${FFMPEG_CONFIGURE_OPTS[@]}"
    ./configure "${FFMPEG_CONFIGURE_OPTS[@]}"
fi
make
make install
cd "$INSTALL_DIR"
