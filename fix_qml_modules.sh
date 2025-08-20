#!/bin/bash

echo "=== Fixing Missing Qt QML Modules ==="
echo

APP_PATH="/Applications/DroidStar_main_77cc798.app"

# We need to find where Qt QML modules are installed
# From the GitHub Actions, Qt should be at /Users/runner/work/DroidStar/Qt/6.5.0/macos
# But on your local machine, we need to find where Qt is installed

echo "1. Looking for Qt installation..."
QT_POSSIBLE_PATHS=(
    "/opt/homebrew/lib/qt6"
    "/opt/homebrew/share/qt"
    "/usr/local/lib/qt6"
    "/usr/local/share/qt"
    "/opt/Qt/6.5.0/macos"
    "/Users/*/Qt/6.5.0/macos"
)

QT_QML_DIR=""
for path in "${QT_POSSIBLE_PATHS[@]}"; do
    if [ -d "$path/qml" ]; then
        QT_QML_DIR="$path/qml"
        echo "✅ Found Qt QML modules at: $QT_QML_DIR"
        break
    fi
done

if [ -z "$QT_QML_DIR" ]; then
    echo "❌ Could not find Qt QML modules. Checking with find..."
    # Try to find QtQuick Controls module specifically
    CONTROLS_PATH=$(find /opt /usr/local /Users -name "qtquickcontrols2plugin.dylib" 2>/dev/null | head -1)
    if [ -n "$CONTROLS_PATH" ]; then
        QT_QML_DIR=$(dirname "$(dirname "$(dirname "$CONTROLS_PATH")")")
        echo "✅ Found Qt QML via plugin search at: $QT_QML_DIR"
    else
        echo "❌ Cannot find Qt QML modules anywhere!"
        echo "Please install Qt6 or tell me where your Qt installation is."
        exit 1
    fi
fi
echo

echo "2. Creating QML directory in app bundle..."
mkdir -p "$APP_PATH/Contents/Resources/qml"
echo

echo "3. Copying essential QML modules..."
# Copy the most commonly needed QML modules
QML_MODULES=("QtQuick" "QtQuick.2" "QtQuick.Controls" "QtQuick.Controls.2" "QtQuick.Layouts" "QtQuick.Window" "QtQuick.Templates" "QtQuick.Templates.2")

for module in "${QML_MODULES[@]}"; do
    if [ -d "$QT_QML_DIR/$module" ]; then
        echo "Copying $module..."
        cp -R "$QT_QML_DIR/$module" "$APP_PATH/Contents/Resources/qml/"
        # Sign the copied plugins
        find "$APP_PATH/Contents/Resources/qml/$module" -name "*.dylib" | while read plugin; do
            codesign --force --sign - "$plugin" 2>/dev/null
        done
    else
        echo "⚠️ Module $module not found in Qt installation"
    fi
done
echo

echo "4. Checking what was copied..."
echo "QML modules now in app:"
ls -la "$APP_PATH/Contents/Resources/qml/" | head -10
echo

echo "5. Verifying QtQuick.Controls plugin specifically..."
CONTROLS_PLUGIN="$APP_PATH/Contents/Resources/qml/QtQuick/Controls/qtquickcontrols2plugin.dylib"
if [ -f "$CONTROLS_PLUGIN" ]; then
    echo "✅ QtQuick Controls plugin found"
    codesign --force --sign - "$CONTROLS_PLUGIN"
    echo "Plugin signed"
elif [ -f "$APP_PATH/Contents/Resources/qml/QtQuick.Controls/qtquickcontrols2plugin.dylib" ]; then
    echo "✅ QtQuick Controls plugin found (alternate path)"
    codesign --force --sign - "$APP_PATH/Contents/Resources/qml/QtQuick.Controls/qtquickcontrols2plugin.dylib"
    echo "Plugin signed"
else
    echo "❌ QtQuick Controls plugin still missing"
    echo "Looking for it manually..."
    find "$APP_PATH/Contents/Resources/qml" -name "*controls*" -type f
fi
echo

echo "6. Re-signing the app bundle..."
codesign --force --sign - "$APP_PATH"
echo

echo "7. Testing app launch..."
echo "Attempting to launch app..."
timeout 10s "$APP_PATH/Contents/MacOS/DroidStar" 2>&1 &
APP_PID=$!
sleep 5
if kill -0 $APP_PID 2>/dev/null; then
    echo "✅ SUCCESS! App is running!"
    kill $APP_PID 2>/dev/null
    echo "App terminated cleanly"
else
    echo "App exited. Checking what error we get now..."
    "$APP_PATH/Contents/MacOS/DroidStar" 2>&1 | head -5
fi

echo
echo "=== QML Module Fix Complete ==="