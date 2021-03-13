#!/usr/bin/env sh
NAME=icon
LOC=./src/icons/$NAME

rm -rf $LOC.iconset
mkdir $LOC.iconset

sips -z 16 16     "$LOC.png" --out "$LOC.iconset/icon_16x16.png"
sips -z 32 32     "$LOC.png" --out "$LOC.iconset/icon_16x16@2x.png"
sips -z 32 32     "$LOC.png" --out "$LOC.iconset/icon_32x32.png"
sips -z 64 64     "$LOC.png" --out "$LOC.iconset/icon_32x32@2x.png"
sips -z 128 128   "$LOC.png" --out "$LOC.iconset/icon_128x128.png"
sips -z 256 256   "$LOC.png" --out "$LOC.iconset/icon_128x128@2x.png"
sips -z 256 256   "$LOC.png" --out "$LOC.iconset/icon_256x256.png"
sips -z 512 512   "$LOC.png" --out "$LOC.iconset/icon_256x256@2x.png"
sips -z 512 512   "$LOC.png" --out "$LOC.iconset/icon_512x512.png"

cp "$LOC.png" "$LOC.iconset/icon_512x512@2x.png"
iconutil -c icns "$LOC.iconset"
rm -R "$LOC.iconset"
