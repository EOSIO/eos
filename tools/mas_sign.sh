#!/bin/bash

set -eu

if [ $# -ne 5 ]; then
   echo Usage: $0 certificate-fingerprint appid provisioning-file keychain-group filename
   exit 1
fi

CERT=$1
APPID=$2
PP=$3
KCG=$4
FILENAME=$5
IFS=. read TEAMID BUNDLEID <<<"${APPID}"

mkdir -p $FILENAME.app/Contents/MacOS
cp "$3" $FILENAME.app/Contents/embedded.provisionprofile
cp $FILENAME $FILENAME.app/Contents/MacOS/$FILENAME

cat > $FILENAME.app/Contents/Info.plist <<ENDENDEND
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleDevelopmentRegion</key>
	<string>en</string>
	<key>CFBundleExecutable</key>
	<string>$FILENAME</string>
	<key>CFBundleIdentifier</key>
	<string>$BUNDLEID</string>
	<key>CFBundleInfoDictionaryVersion</key>
	<string>6.0</string>
	<key>CFBundleName</key>
	<string>$FILENAME</string>
	<key>CFBundlePackageType</key>
	<string>APPL</string>
	<key>CFBundleShortVersionString</key>
	<string>1.0</string>
	<key>CFBundleSupportedPlatforms</key>
	<array>
		<string>MacOSX</string>
	</array>
	<key>CFBundleVersion</key>
	<string>1</string>
</dict>
</plist>
ENDENDEND

TMPFILE=$(mktemp /tmp/signscript.XXXXXX)
trap "rm $TMPFILE" EXIT

cat > $TMPFILE <<ENDENDEND
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>com.apple.application-identifier</key>
	<string>$APPID</string>
	<key>com.apple.developer.team-identifier</key>
	<string>$TEAMID</string>
	<key>keychain-access-groups</key>
	<array>
		<string>$KCG</string>
	</array>
</dict>
</plist>
ENDENDEND

codesign --force --sign $CERT --timestamp=none --entitlements $TMPFILE $FILENAME.app
