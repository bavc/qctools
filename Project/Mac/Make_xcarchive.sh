APPNAME=$1
VERSION=$2
BUNDLEID=$3
TEAMID=$4

SIGNATURE="MediaArea.net"

DATE=$(date -u +'%s')

cp -r ../QtCreator/qctools-gui/$APPNAME.app . || exit 1

macdeployqt $APPNAME.app -no-strip -appstore-compliant
rm -rf $APPNAME.app/Contents/PlugIns/sqldrivers/{libqsqlmysql.dylib,libqsqlodbc.dylib,libqsqlpsql.dylib}

dsymutil $APPNAME.app/Contents/MacOS/$APPNAME -o $APPNAME.app.dSYM

for FRAMEWORK in $(ls $APPNAME.app/Contents/Frameworks | grep framework | sed 's/\.framework//') ; do
    pushd $APPNAME.app/Contents/Frameworks/$FRAMEWORK.framework

    rm -fr _CodeSignature
    rm -fr Versions/5/_CodeSignature
    plutil -replace CFBundleIdentifier -string "$BUNDLEID" Resources/Info.plist
    popd

 #   codesign --force --verbose --sign "3rd Party Mac Developer Application: $SIGNATURE" -i $BUNDLEID $APPNAME.app/Contents/Frameworks/$FRAMEWORK.framework/Versions/5/$FRAMEWORK
done

#find $APPNAME.app/Contents/PlugIns -name "*.dylib" -exec codesign --force --verbose --sign "3rd Party Mac Developer Application: $SIGNATURE" -i $BUNDLEID '{}' \;

codesign --deep --force --verbose --sign "3rd Party Mac Developer Application: $SIGNATURE"  --entitlements $APPNAME.entitlements $APPNAME.app

productbuild --component $APPNAME.app /Applications --sign "3rd Party Mac Developer Installer: $SIGNATURE" $APPNAME-$VERSION.pkg

rm -fr $APPNAME.xcarchive
mkdir -p $APPNAME.xcarchive{/Products/Applications,/dSYMs}
mv $APPNAME.app.dSYM $APPNAME.xcarchive/dSYMs
mv $APPNAME.app $APPNAME.xcarchive/Products/Applications

cat > $APPNAME.xcarchive/Info.plist <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>ApplicationProperties</key>
	<dict>
		<key>ApplicationPath</key>
		<string>Applications/$APPNAME.app</string>
		<key>CFBundleIdentifier</key>
		<string>$BUNDLEID</string>
		<key>CFBundleShortVersionString</key>
		<string>$VERSION</string>
		<key>CFBundleVersion</key>
		<string>1.3.1</string>
		<key>IconPaths</key>
		<array>
			<string>Applications/$APPNAME.app/Contents/Resources/Logo.icns</string>
		</array>
		<key>SigningIdentity</key>
		<string>3rd Party Mac Developer Application: $SIGNATURE ($TEAMID)</string>
	</dict>
	<key>ArchiveVersion</key>
	<integer>2</integer>
	<key>CreationDate</key>
	<date>$(date -u -j -f '%s' +'%Y-%m-%dT%H:%M:%SZ' $DATE)</date>
	<key>Name</key>
	<string>$APPNAME</string>
	<key>SchemeName</key>
	<string>$APPNAME</string>
</dict>
</plist>
EOF

mkdir -p $HOME/Library/Developer/Xcode/Archives/$(date -u -j -f '%s' +'%Y-%m-%d' $DATE)
cp -a $APPNAME.xcarchive $HOME/Library/Developer/Xcode/Archives/$(date -u -j -f '%s' +'%Y-%m-%d' $DATE)/"$APPNAME $(date -u -j -f '%s' +'%d-%m-%Y %H.%M' $DATE).xcarchive"
