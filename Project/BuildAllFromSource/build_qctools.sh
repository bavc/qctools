cd qctools/Project/QtCreator
if sw_vers >/dev/null 2>&1 ; then
    #Qt 5.2 can not well detect the platform version for 10.9 then FFmpeg static link fails
    MAJOR_MAC_VERSION=$(sw_vers -productVersion | awk -F '.' '{print $1 "." $2}')
    $BINQMAKE QMAKE_MAC_SDK=macosx$MAJOR_MAC_VERSION
else
    $BINQMAKE
fi
make
cd ../../..

echo QCTools binary is in qctools/Project/QtCreator
