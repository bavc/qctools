Building
--------
Xcode needs to be installed before proceeding with your build. https://developer.apple.com/xcode/download/

Install ffmpeg, freetype2, qt

    $ brew install freetype ffmpeg qt qwt

set QCTOOLS_USE_BREW environment variable:

    $ export QCTOOLS_USE_BREW=true

Build the main application

    $ qmake
    $ make
