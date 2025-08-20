#!/bin/bash

echo "=== Checking DroidStar App Bundle Framework Issues ==="
echo

APP_PATH="/Applications/DroidStar_main_77cc798.app"

echo "1. Checking QtCore framework structure:"
echo "QtCore framework root:"
ls -la "$APP_PATH/Contents/Frameworks/QtCore.framework/"
echo
echo "QtCore Versions directory:"
ls -la "$APP_PATH/Contents/Frameworks/QtCore.framework/Versions/"
echo
echo "QtCore Versions/A directory:"
ls -la "$APP_PATH/Contents/Frameworks/QtCore.framework/Versions/A/"
echo

echo "2. Checking other frameworks:"
echo "QtGui framework Versions/A:"
ls -la "$APP_PATH/Contents/Frameworks/QtGui.framework/Versions/A/"
echo
echo "QtQml framework Versions/A:"
ls -la "$APP_PATH/Contents/Frameworks/QtQml.framework/Versions/A/"
echo

echo "3. Checking if framework binaries exist:"
for framework in QtCore QtGui QtQml QtQuick QtNetwork QtMultimedia QtSerialPort; do
    FRAMEWORK_BINARY="$APP_PATH/Contents/Frameworks/$framework.framework/Versions/A/$framework"
    if [ -f "$FRAMEWORK_BINARY" ]; then
        echo "✅ $framework binary exists ($(du -h "$FRAMEWORK_BINARY" | cut -f1))"
        file "$FRAMEWORK_BINARY"
    else
        echo "❌ $framework binary MISSING at: $FRAMEWORK_BINARY"
    fi
done
echo

echo "4. Checking framework symlinks:"
for framework in QtCore QtGui QtQml; do
    FRAMEWORK_DIR="$APP_PATH/Contents/Frameworks/$framework.framework"
    echo "Framework: $framework"
    echo "  Root symlink:"
    ls -la "$FRAMEWORK_DIR/$framework" 2>/dev/null || echo "  ❌ Root symlink missing"
    echo "  Current symlink:"
    ls -la "$FRAMEWORK_DIR/Versions/Current" 2>/dev/null || echo "  ❌ Current symlink missing"
    echo
done

echo "5. Code signing check:"
codesign -dv "$APP_PATH" 2>&1 || echo "No code signing info"
echo

echo "6. Try running the app to see actual error:"
echo "Running: $APP_PATH/Contents/MacOS/DroidStar"
timeout 5 "$APP_PATH/Contents/MacOS/DroidStar" 2>&1 || echo "App failed to start or timed out"