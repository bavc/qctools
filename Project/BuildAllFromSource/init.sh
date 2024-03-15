#! /bin/bash

if qmake --version >/dev/null 2>&1 ; then
    BINQMAKE=qmake
elif qmake-qt5 --version >/dev/null 2>&1 ; then
    BINQMAKE=qmake-qt5
elif qmake5 --version >/dev/null 2>&1 ; then
    BINQMAKE=qmake5
else
    echo qmake not found
    exit
fi

echo "PWD: " + $PWD

START_DIR=$(cd $(dirname "$0") && pwd)
INSTALL_DIR=$(cd .. && pwd)
SCRIPT_DIR=$PWD/Project/BuildAllFromSource

echo "SCRIPT_DIR: " + $SCRIPT_DIR
echo "INSTALL_DIR: " + $INSTALL_DIR
echo "START_DIR: " + $START_DIR

echo "BINQMAKE: " + $BINQMAKE
