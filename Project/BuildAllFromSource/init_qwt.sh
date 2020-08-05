#! /bin/bash

echo "PWD: " + $PWD

if [ ! -d qwt ] ; then
    wget https://github.com/ElderOrb/qwt/archive/v6.1.5.zip
    unzip v6.1.5.zip
    mv qwt-6.1.5 qwt
    rm v6.1.5.zip
fi
cd qwt
$BINQMAKE
make
cd "$INSTALL_DIR"
