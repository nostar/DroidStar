#!/bin/bash

echo "=== Fixing DroidStar Code Signing Issues ==="
echo

APP_PATH="/Applications/DroidStar_main_77cc798.app"

echo "1. Current code signing status:"
codesign -dv "$APP_PATH" 2>&1
echo

echo "2. Checking librtmidi signing:"
codesign -dv "$APP_PATH/Contents/Frameworks/librtmidi.7.dylib" 2>&1
echo

echo "3. Re-signing librtmidi.7.dylib with adhoc signature:"
codesign --force --sign - "$APP_PATH/Contents/Frameworks/librtmidi.7.dylib"
echo "Result: $?"
echo

echo "4. Re-signing all Qt frameworks:"
for framework in QtCore QtGui QtQml QtQuick QtQuickControls2 QtQmlModels QtMultimedia QtNetwork QtSerialPort QtOpenGL QtDBus QtQuickTemplates2 QtSvg QtWidgets; do
    FRAMEWORK_PATH="$APP_PATH/Contents/Frameworks/$framework.framework"
    if [ -d "$FRAMEWORK_PATH" ]; then
        echo "Re-signing $framework..."
        codesign --force --sign - "$FRAMEWORK_PATH"
        echo "Result: $?"
    fi
done
echo

echo "5. Re-signing all plugins:"
find "$APP_PATH/Contents/PlugIns" -name "*.dylib" | while read plugin; do
    echo "Re-signing $(basename "$plugin")..."
    codesign --force --sign - "$plugin"
    echo "Result: $?"
done
echo

echo "6. Re-signing the main app bundle:"
codesign --force --sign - "$APP_PATH"
echo "Result: $?"
echo

echo "7. Verifying the fix:"
echo "Main app verification:"
codesign -dv "$APP_PATH" 2>&1
echo
echo "librtmidi verification:"
codesign -dv "$APP_PATH/Contents/Frameworks/librtmidi.7.dylib" 2>&1
echo

echo "8. Testing app launch:"
echo "Attempting to launch app..."
timeout 5s "$APP_PATH/Contents/MacOS/DroidStar" 2>&1 &
APP_PID=$!
sleep 3
if kill -0 $APP_PID 2>/dev/null; then
    echo "✅ SUCCESS! App is running!"
    kill $APP_PID 2>/dev/null
    echo "App terminated cleanly"
else
    echo "❌ App still not starting. Checking logs..."
    log show --last 1m --predicate 'process == "DroidStar"' --info --debug | tail -5
fi

echo
echo "=== Code Signing Fix Complete ==="