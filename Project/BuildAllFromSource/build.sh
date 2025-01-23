#! /bin/bash

###################################################################################################
# build.sh - Batch script for building the Unix version of QCTools                                #
#                                                                                                 #
# Script requirements:                                                                            #
# - qctools_AllInclusive source                                                                   #
# - Qt bin directory in the PATH                                                                  #
# - Python3  binary in the PATH                                                                   #
# - pkg-config binary in the PATH                                                                 #
# - Meson binary in the PATH                                                                      #
# - Ninja binary in the PATH                                                                      #
# - Git binary in the PATH                                                                        #
# Environment:                                                                                    #
# - MULTIARCH: Compile for arm64 and x86_64 into the same binary (macOS)                          #
###################################################################################################
# setup environment
set -e

if qmake --version >/dev/null 2>&1 ; then
    BINQMAKE=qmake
elif qmake-qt6 --version >/dev/null 2>&1 ; then
    BINQMAKE=qmake-qt6
elif qmake6 --version >/dev/null 2>&1 ; then
    BINQMAKE=qmake6
else
    echo qmake not found
    exit 1
fi

SCRIPT_DIR=$(cd $(dirname "$0") && pwd)
INSTALL_DIR=$(cd $(dirname "$0") && cd ../../.. && pwd)
cd "$INSTALL_DIR"

FFMPEG_CONFIGURE_OPTS=(
    --enable-gpl
    --enable-version3
    --disable-doc
    --disable-debug
    --disable-programs
    --disable-autodetect
    --enable-static
    --disable-shared
    --enable-libfreetype
    --enable-libharfbuzz
)

QT_CONFIGURE_OPTS=()

if sw_vers >/dev/null 2>&1 ; then
    OS=mac
    export CFLAGS="-mmacosx-version-min=11.0 $CFLAGS"
    export CXXFLAGS="-mmacosx-version-min=11.0 $CXXFLAGS"
    export LDFLAGS="-mmacosx-version-min=11.0 $LDFLAGS"
    FFMPEG_CONFIGURE_OPTS+=(--extra-cflags="-mmacosx-version-min=11.0" --extra-ldflags="-mmacosx-version-min=11.0")
    if [[ -n "$MULTIARCH" ]] ; then
        QT_CONFIGURE_OPTS+=(QMAKE_APPLE_DEVICE_ARCHS="x86_64 arm64")
        mkdir -p $INSTALL_DIR/output/lib
    fi
fi

# get dependencies
while read LINE; do
    DIR=$(echo "$LINE" | cut -d: -f1)
    URL=$(echo "$LINE" | cut -d: -f2-)
    if [[ -z "$DIR" || -z "$URL" ]] ; then
        continue
    fi
    if [[ ! -e "$DIR" ]] ; then
        mkdir "$DIR"
        pushd "$DIR"
            curl -LO "$URL"
            tar --extract --strip-components=1 --file=${URL##*/}
            rm -f ${URL##*/}
        popd   
    fi
done < $SCRIPT_DIR/dependencies.txt

# freetype
echo "Build FreeType"
if [[ -d freetype/build ]] ; then
    rm -fr freetype/build
fi
mkdir freetype/build
pushd freetype/build
    if [[ "$OS" == "mac" && -n "$MULTIARCH" ]] ; then
        mkdir x86_64
        pushd x86_64
        (
            export PKG_CONFIG_PATH=$INSTALL_DIR/output/x86_64/lib/pkgconfig
            export CFLAGS="$CFLAGS -arch x86_64"
            export CXXFLAGS="$CXXFLAGS -arch x86_64"
            export LDFLAGS="$LDFLAGS -arch x86_64"
            meson setup --prefix $INSTALL_DIR/output/x86_64 --default-library=static -Dzlib=internal -Dbzip2=disabled -Dpng=disabled -Dharfbuzz=disabled -Dbrotli=disabled ../..
            ninja install
        )
        popd
        mkdir arm64
        pushd arm64
        (
            export PKG_CONFIG_PATH=$INSTALL_DIR/output/arm64/lib/pkgconfig
            export CFLAGS="$CFLAGS -arch arm64"
            export CXXFLAGS="$CXXFLAGS -arch arm64"
            export LDFLAGS="$LDFLAGS -arch arm64"
            meson setup --prefix $INSTALL_DIR/output/arm64 --default-library=static -Dzlib=disabled -Dbzip2=disabled -Dpng=disabled -Dharfbuzz=disabled -Dbrotli=disabled ../..
            ninja install
        )
        popd
        lipo -create $INSTALL_DIR/output/x86_64/lib/libfreetype.a $INSTALL_DIR/output/arm64/lib/libfreetype.a -output $INSTALL_DIR/output/lib/libfreetype.a
    else
    (
        export PKG_CONFIG_PATH=$INSTALL_DIR/output/lib/pkgconfig
        meson setup --prefix $INSTALL_DIR/output --default-library=static -Dzlib=disabled -Dbzip2=disabled -Dpng=disabled -Dharfbuzz=disabled -Dbrotli=disabled ..
        ninja install
    )
    fi
popd

# harfbuzz
echo "Build HarfBuzz"
if [[ -d harfbuzz/build ]] ; then
    rm -fr harfbuzz/build
fi
mkdir harfbuzz/build
pushd harfbuzz/build
    if [[ "$OS" == "mac" && -n "$MULTIARCH" ]] ; then
        mkdir x86_64
        pushd x86_64
        (
            export PKG_CONFIG_PATH=$INSTALL_DIR/output/x86_64/lib/pkgconfig
            export CFLAGS="$CFLAGS -arch x86_64"
            export CXXFLAGS="$CXXFLAGS -arch x86_64"
            export LDFLAGS="$LDFLAGS -arch x86_64"
            meson setup --prefix $INSTALL_DIR/output/x86_64 --default-library=static -Dglib=disabled -Dgobject=disabled -Dcairo=disabled -Dchafa=disabled -Dicu=disabled -Dgraphite=disabled -Dgraphite2=disabled -Dgdi=disabled -Ddirectwrite=disabled -Dcoretext=disabled -Dwasm=disabled -Dtests=disabled -Dintrospection=disabled -Ddocs=disabled -Ddoc_tests=false -Dutilities=disabled ../..
            ninja install
        )
        popd
        mkdir arm64
        pushd arm64
        (
            export PKG_CONFIG_PATH=$INSTALL_DIR/output/arm64/lib/pkgconfig
            export CFLAGS="$CFLAGS -arch arm64"
            export CXXFLAGS="$CXXFLAGS -arch arm64"
            export LDFLAGS="$LDFLAGS -arch arm64"
            meson setup --prefix $INSTALL_DIR/output/arm64 --default-library=static -Dglib=disabled -Dgobject=disabled -Dcairo=disabled -Dchafa=disabled -Dicu=disabled -Dgraphite=disabled -Dgraphite2=disabled -Dgdi=disabled -Ddirectwrite=disabled -Dcoretext=disabled -Dwasm=disabled -Dtests=disabled -Dintrospection=disabled -Ddocs=disabled -Ddoc_tests=false -Dutilities=disabled ../..
            ninja install
        )
        popd
        lipo -create $INSTALL_DIR/output/x86_64/lib/libharfbuzz.a $INSTALL_DIR/output/arm64/lib/libharfbuzz.a -output $INSTALL_DIR/output/lib/libharfbuzz.a
    else
    (
        export PKG_CONFIG_PATH=$INSTALL_DIR/output/lib/pkgconfig
        meson setup --prefix $INSTALL_DIR/output --default-library=static -Dglib=disabled -Dgobject=disabled -Dcairo=disabled -Dchafa=disabled -Dicu=disabled -Dgraphite=disabled -Dgraphite2=disabled -Dgdi=disabled -Ddirectwrite=disabled -Dcoretext=disabled -Dwasm=disabled -Dtests=disabled -Dintrospection=disabled -Ddocs=disabled -Ddoc_tests=false -Dutilities=disabled ..
        ninja install
    )
    fi
popd

# ffmpeg
echo "Build FFmpeg"
if [[ -d ffmpeg/build ]] ; then
    rm -fr ffmpeg/build
fi
mkdir ffmpeg/build
pushd ffmpeg/build
    if [[ "$OS" == "mac" && -n "$MULTIARCH" ]] ; then
        mkdir x86_64
        pushd x86_64
        (
            export PKG_CONFIG_PATH=$INSTALL_DIR/output/x86_64/lib/pkgconfig
            ../../configure --arch=x86_64 --extra-cflags="-arch x86_64" --extra-ldflags="-arch x86_64" --prefix=$INSTALL_DIR/output/x86_64 "${FFMPEG_CONFIGURE_OPTS[@]}"
            make install
        )
        popd
        mkdir arm64
        pushd arm64
        (
            export PKG_CONFIG_PATH=$INSTALL_DIR/output/arm64/lib/pkgconfig
            ../../configure --arch=arm64 --extra-cflags="-arch arm64" --extra-ldflags="-arch arm64" --prefix=$INSTALL_DIR/output/arm64 "${FFMPEG_CONFIGURE_OPTS[@]}"
            make install
        )
        popd
        lipo -create $INSTALL_DIR/output/x86_64/lib/libavcodec.a $INSTALL_DIR/output/arm64/lib/libavcodec.a -output $INSTALL_DIR/output/lib/libavcodec.a
        lipo -create $INSTALL_DIR/output/x86_64/lib/libavdevice.a $INSTALL_DIR/output/arm64/lib/libavdevice.a -output $INSTALL_DIR/output/lib/libavdevice.a
        lipo -create $INSTALL_DIR/output/x86_64/lib/libavfilter.a $INSTALL_DIR/output/arm64/lib/libavfilter.a -output $INSTALL_DIR/output/lib/libavfilter.a
        lipo -create $INSTALL_DIR/output/x86_64/lib/libavformat.a $INSTALL_DIR/output/arm64/lib/libavformat.a -output $INSTALL_DIR/output/lib/libavformat.a
        lipo -create $INSTALL_DIR/output/x86_64/lib/libavutil.a $INSTALL_DIR/output/arm64/lib/libavutil.a -output $INSTALL_DIR/output/lib/libavutil.a
        lipo -create $INSTALL_DIR/output/x86_64/lib/libpostproc.a $INSTALL_DIR/output/arm64/lib/libpostproc.a -output $INSTALL_DIR/output/lib/libpostproc.a
        lipo -create $INSTALL_DIR/output/x86_64/lib/libswresample.a $INSTALL_DIR/output/arm64/lib/libswresample.a -output $INSTALL_DIR/output/lib/libswresample.a
        lipo -create $INSTALL_DIR/output/x86_64/lib/libswscale.a $INSTALL_DIR/output/arm64/lib/libswscale.a -output $INSTALL_DIR/output/lib/libswscale.a
        cp -r $INSTALL_DIR/output/x86_64/include $INSTALL_DIR/output
    else
    (
        export PKG_CONFIG_PATH=$INSTALL_DIR/output/lib/pkgconfig
        ../configure --prefix=$INSTALL_DIR/output "${FFMPEG_CONFIGURE_OPTS[@]}"
        make install
    )
    fi
popd

# qwt
echo "Build Qwt"
if [[ -d qwt/build ]] ; then
    rm -fr qwt/build
fi
mkdir qwt/build
pushd qwt/build
(
    git -C .. apply "$SCRIPT_DIR/qwt.patch"
    export QWT_STATIC=1 QWT_NO_SVG=1 QWT_NO_OPENGL=1 QWT_NO_DESIGNER=1 QWT_NO_EXAMPLES=1 QWT_NO_PLAYGROUND=1 QWT_NO_TESTS=1 QWT_INSTALL_PREFIX=$INSTALL_DIR/output
    $BINQMAKE "${QT_CONFIGURE_OPTS[@]}" ..
    make install
)
popd

# qctools
echo "Build QCTools"
if [[ -d qctools/Project/QtCreator/build ]] ; then
    rm -fr qctools/Project/QtCreator/build
fi
mkdir qctools/Project/QtCreator/build
pushd qctools/Project/QtCreator/build
(
    export QWT_ROOT=$INSTALL_DIR/output FFMPEG=$INSTALL_DIR/output
    $BINQMAKE "${QT_CONFIGURE_OPTS[@]}" STATIC=1 ..
    make
)
popd

echo QCTools binary is in $INSTALL_DIR/qctools/Project/QtCreator/build/qctools-gui
echo qcli binary is in $INSTALL_DIR/qctools/Project/QtCreator/build/qctools-cli
