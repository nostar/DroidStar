#!/bin/bash

echo "=== DroidStar App Launch Debug Script ==="
echo

APP_PATH="/Applications/DroidStar_main_77cc798.app"
BINARY_PATH="$APP_PATH/Contents/MacOS/DroidStar"

echo "1. Checking Qt framework dependencies:"
echo "QtCore framework dependencies:"
otool -L "$APP_PATH/Contents/Frameworks/QtCore.framework/Versions/A/QtCore" | head -10
echo
echo "QtGui framework dependencies:"
otool -L "$APP_PATH/Contents/Frameworks/QtGui.framework/Versions/A/QtGui" | head -10
echo

echo "2. Checking app bundle resources:"
echo "QML resources:"
if [ -d "$APP_PATH/Contents/Resources/qml/" ]; then
    ls -la "$APP_PATH/Contents/Resources/qml/"
else
    echo "❌ No QML resources directory found"
fi
echo

echo "QML files in app:"
find "$APP_PATH" -name "*.qml" | head -10 || echo "No QML files found"
echo

echo "qt.conf file:"
if [ -f "$APP_PATH/Contents/Resources/qt.conf" ]; then
    echo "✅ qt.conf exists:"
    cat "$APP_PATH/Contents/Resources/qt.conf"
else
    echo "❌ No qt.conf found"
fi
echo

echo "3. Checking for missing Qt plugins:"
echo "Available Qt plugins:"
find "$APP_PATH/Contents/PlugIns" -name "*.dylib" | head -10 || echo "No plugins found"
echo

echo "4. Trying to launch with debug output:"
echo "Setting Qt debug environment variables..."
export QT_DEBUG_PLUGINS=1
export QT_LOGGING_RULES="*=true"
export QT_QPA_PLATFORM_PLUGIN_PATH="$APP_PATH/Contents/PlugIns/platforms"

echo "Attempting to launch app with timeout..."
echo "Command: timeout 10s '$BINARY_PATH'"
echo "--- App Output Start ---"
timeout 10s "$BINARY_PATH" 2>&1 || echo "--- App failed or timed out ---"
echo "--- App Output End ---"
echo

echo "5. Checking system logs for crash info:"
echo "Searching recent logs for DroidStar crashes..."
echo "--- System Log Output Start ---"
log show --last 5m --predicate 'process == "DroidStar" OR eventMessage CONTAINS "DroidStar"' --info --debug 2>/dev/null | tail -20 || echo "No recent DroidStar logs found"
echo "--- System Log Output End ---"
echo

echo "6. Checking for codesigning issues:"
echo "Detailed codesigning verification:"
codesign -vvv "$APP_PATH" 2>&1 || echo "Codesign verification failed"
echo

echo "7. Checking for library loading issues:"
echo "Checking if all framework binaries can be loaded:"
for framework in QtCore QtGui QtQml QtQuick QtNetwork QtMultimedia QtSerialPort; do
    FRAMEWORK_BINARY="$APP_PATH/Contents/Frameworks/$framework.framework/Versions/A/$framework"
    if [ -f "$FRAMEWORK_BINARY" ]; then
        # Try to get basic info to see if library can be loaded
        otool -h "$FRAMEWORK_BINARY" > /dev/null 2>&1 && echo "✅ $framework can be inspected" || echo "❌ $framework appears corrupted"
    fi
done
echo

echo "8. Checking system requirements:"
echo "macOS version:"
sw_vers
echo "Architecture compatibility:"
uname -m
echo "Available architectures for main binary:"
lipo -info "$BINARY_PATH"
echo

echo "9. Final attempt with crash detection:"
echo "Trying to run app and capture any crash logs..."
echo "Running: '$BINARY_PATH' &"
"$BINARY_PATH" &
APP_PID=$!
echo "App PID: $APP_PID"
sleep 3
if kill -0 $APP_PID 2>/dev/null; then
    echo "✅ App is running! Terminating..."
    kill $APP_PID 2>/dev/null
    echo "App terminated normally"
else
    echo "❌ App crashed or failed to start"
    echo "Checking for crash reports..."
    ls -la ~/Library/Logs/DiagnosticReports/*DroidStar* 2>/dev/null | tail -3 || echo "No crash reports found"
fi
echo

echo "=== Debug Script Complete ==="