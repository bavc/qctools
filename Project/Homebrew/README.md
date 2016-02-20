Building
--------

Install ffmpeg, freetype2, qt5

    $ brew install freetype ffmpeg qt5

Uninstall qwt if already installed and reinstall from provided formula (this might
require not having a homebrew installation of qt (aka qt4):

    $ brew uninstall qwt
    $ brew install qwt-qt5.rb

Build the main application

    $ qmake
    $ make

Caveats
-------

Building qwt with qt4 installed was not tested and might not work.
