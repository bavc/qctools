#! /bin/bash

echo "PWD: " + $PWD

START_DIR=$(cd $(dirname "$0") && pwd)
INSTALL_DIR=$(cd .. && pwd)
SCRIPT_DIR=$PWD/Project/BuildAllFromSource

echo "SCRIPT_DIR: " + $SCRIPT_DIR
echo "INSTALL_DIR: " + $INSTALL_DIR
echo "START_DIR: " + $START_DIR

BINQMAKE=qmake
echo "BINQMAKE: " + $BINQMAKE
