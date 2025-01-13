APPNAME=$1
VERSION=$2
BUNDLEID=$3
TEAMID=$4

SIGNATURE="MediaArea.net"

cp -r ../QtCreator/qctools-gui/build/$APPNAME.app . || exit 1

codesign --force --deep --verbose --sign "Apple Distribution: $SIGNATURE"  --entitlements "$APPNAME.entitlements" "$APPNAME.app"
productbuild --component "$APPNAME.app" /Applications --sign "3rd Party Mac Developer Installer: $SIGNATURE" "$APPNAME-$VERSION.pkg"
