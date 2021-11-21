#!/bin/sh

# This scripts regenerate the iconset for the MacOS app

icon=droidstar.png
iconset=droidstar.iconset

rm -rf "$iconset"
mkdir "$iconset"

sips -z 16 16     "$icon" --out "$iconset"/icon_16x16.png
sips -z 32 32     "$icon" --out "$iconset"/icon_16x16@2x.png
sips -z 32 32     "$icon" --out "$iconset"/icon_32x32.png
sips -z 64 64     "$icon" --out "$iconset"/icon_32x32@2x.png
sips -z 128 128   "$icon" --out "$iconset"/icon_128x128.png
sips -z 256 256   "$icon" --out "$iconset"/icon_128x128@2x.png
sips -z 256 256   "$icon" --out "$iconset"/icon_256x256.png
sips -z 512 512   "$icon" --out "$iconset"/icon_256x256@2x.png
sips -z 512 512   "$icon" --out "$iconset"/icon_512x512.png
sips -z 1024 1024   "$icon" --out "$iconset"/icon_512x512@2.png

iconutil -c icns "$iconset"
