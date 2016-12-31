Building
--------
Xcode needs to be installed before proceeding with your build. https://developer.apple.com/xcode/download/

Install ffmpeg, freetype2, qt5

    $ brew install freetype ffmpeg qt5

Uninstall qwt if already installed and reinstall from provided formula (this might
require not having a homebrew installation of qt (aka qt4):

    $ brew uninstall qwt
    $ brew install qwt-qt5.rb

set QCTOOLS_USE_BREW environment variable:

    $ export QCTOOLS_USE_BREW=true

Build the main application

    $ qmake
    $ make

Caveats
-------

Building qwt with qt4 installed was not tested and might not work.
