#!/bin/bash

set -eu

if [ $# -ne 3 ]; then
   echo Usage: $0 certificate-fingerprint appid provisioning-file
   exit 1
fi

CERT=$1
APPID=$2
PP=$3
IFS=. read TEAMID BUNDLEID <<<"${APPID}"

mkdir -p applesedemo.app/Contents/MacOS
cp "$3" applesedemo.app/Contents/embedded.provisionprofile
cp applesedemo applesedemo.app/Contents/MacOS/applesedemo

cat > applesedemo.app/Contents/Info.plist <<ENDENDEND
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleDevelopmentRegion</key>
	<string>en</string>
	<key>CFBundleExecutable</key>
	<string>applesedemo</string>
	<key>CFBundleIdentifier</key>
	<string>$BUNDLEID</string>
	<key>CFBundleInfoDictionaryVersion</key>
	<string>6.0</string>
	<key>CFBundleName</key>
	<string>applesedemo</string>
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
	<key>com.apple.security.app-sandbox</key>
	<true/>
	<key>com.apple.security.files.user-selected.read-only</key>
	<true/>
</dict>
</plist>
ENDENDEND

codesign --force --sign $CERT --timestamp=none --entitlements $TMPFILE applesedemo.app
