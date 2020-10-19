Building
--------
Xcode needs to be installed before proceeding with your build. https://developer.apple.com/xcode/download/

Install ffmpeg, freetype2, qt, qwt

    $ brew install freetype ffmpeg qt qwt

Install submodules

    $ git submodule update --init --recursive

set QCTOOLS_USE_BREW environment variable:

    $ export QCTOOLS_USE_BREW=true

Build the main application

    $ cd Project/QtCreator
    $ mkdir build
    $ cd build
    $ qmake ../
    $ make

Or from within the build directory, run:

    $ export QCTOOLS_USE_BREW=true ; qmake ../ && make
