APPNAME=$1
VERSION=$2
BUNDLEID=$3
TEAMID=$4
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

SIGNATURE="MediaArea.net"
FRAMEWORKS="${APPNAME}.app/Contents/Frameworks"
EXECUTABLE="${APPNAME}.app/Contents/MacOS/${APPNAME}"

cp -r ../QtCreator/qctools-gui/build/$APPNAME.app . || exit 1

. "${SCRIPT_DIR}/ffmpeg_dylibs.sh"
bundle_ffmpeg_dylibs "${FRAMEWORKS}" "@executable_path/../Frameworks" "${EXECUTABLE}" || exit 1

for DYLIB in "${FRAMEWORKS}"/*.dylib ; do
    codesign --force --verbose --sign "Apple Distribution: $SIGNATURE" "$DYLIB"
done
codesign --force --deep --verbose --sign "Apple Distribution: $SIGNATURE"  --entitlements "$APPNAME.entitlements" "$APPNAME.app"
productbuild --component "$APPNAME.app" /Applications --sign "3rd Party Mac Developer Installer: $SIGNATURE" "$APPNAME-$VERSION.pkg"
