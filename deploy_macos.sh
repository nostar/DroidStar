#!/bin/bash
#
# macOS Deployment Script for DroidStar
# Creates a portable, self-contained macOS application bundle
#
# Usage: ./deploy_macos.sh [options]
# Options:
#   -h, --help          Show this help message
#   -c, --codesign ID   Code sign with given identity
#   -n, --notarize ID   Sign for notarization (requires Apple Developer ID)
#   -d, --dmg           Create DMG disk image (default)
#   --no-dmg            Skip DMG creation
#   --clean             Clean build directory first
#

set -e  # Exit on any error

# Default options
CREATE_DMG=true
CLEAN_BUILD=false
CODESIGN_ID=""
NOTARIZE_ID=""

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            head -n 20 "$0" | tail -n +2 | sed 's/^#//'
            exit 0
            ;;
        -c|--codesign)
            CODESIGN_ID="$2"
            shift 2
            ;;
        -n|--notarize)
            NOTARIZE_ID="$2"
            shift 2
            ;;
        -d|--dmg)
            CREATE_DMG=true
            shift
            ;;
        --no-dmg)
            CREATE_DMG=false
            shift
            ;;
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Check for required tools
command -v cmake >/dev/null 2>&1 || { echo "cmake is required but not installed. Run: brew install cmake"; exit 1; }
command -v make >/dev/null 2>&1 || { echo "make is required but not found"; exit 1; }
command -v macdeployqt >/dev/null 2>&1 || { echo "macdeployqt is required but not found. Run: brew install qt"; exit 1; }

echo "🚀 Starting DroidStar macOS deployment..."

# Clean build if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo "🧹 Cleaning build directory..."
    rm -rf build
fi

# Create build directory and build
echo "🔨 Building DroidStar..."
mkdir -p build
cd build

if [ ! -f Makefile ]; then
    echo "⚙️  Configuring with CMake..."
    cmake ..
fi

echo "🏗️  Compiling (using $(sysctl -n hw.ncpu) cores)..."
make -j$(sysctl -n hw.ncpu)

# Remove any existing deployed app
rm -rf DroidStar_Portable.app DroidStar_Portable.dmg

# Create portable app bundle
echo "📦 Creating portable app bundle..."
cp -r DroidStar.app DroidStar_Portable.app

# Deploy Qt and other dependencies
echo "🔧 Deploying Qt frameworks and dependencies..."
if [ -n "$CODESIGN_ID" ]; then
    if [ -n "$NOTARIZE_ID" ]; then
        echo "🔐 Signing for notarization with identity: $NOTARIZE_ID"
        macdeployqt DroidStar_Portable.app -sign-for-notarization="$NOTARIZE_ID" -timestamp
    else
        echo "✏️  Code signing with identity: $CODESIGN_ID"
        macdeployqt DroidStar_Portable.app -codesign="$CODESIGN_ID"
    fi
else
    macdeployqt DroidStar_Portable.app
fi

# Check that deployment was successful
if [ ! -d "DroidStar_Portable.app/Contents/Frameworks" ]; then
    echo "❌ Error: Deployment failed - no frameworks directory found"
    exit 1
fi

echo "✅ Deployment successful!"

# Show app info
APP_SIZE=$(du -sh DroidStar_Portable.app | awk '{print $1}')
echo "📊 Portable app size: $APP_SIZE"

# Verify dependencies are bundled
echo "🔍 Verifying dependencies..."
EXTERNAL_DEPS=$(otool -L DroidStar_Portable.app/Contents/MacOS/DroidStar | grep -v "@executable_path" | grep -v "/usr/lib" | grep -v "/System/Library" | wc -l)
if [ "$EXTERNAL_DEPS" -gt 0 ]; then
    echo "⚠️  Warning: Found external dependencies that may not be portable:"
    otool -L DroidStar_Portable.app/Contents/MacOS/DroidStar | grep -v "@executable_path" | grep -v "/usr/lib" | grep -v "/System/Library"
else
    echo "✅ All dependencies are bundled - app should be portable"
fi

# Create DMG if requested
if [ "$CREATE_DMG" = true ]; then
    echo "💿 Creating DMG distribution package..."
    if [ -n "$CODESIGN_ID" ]; then
        macdeployqt DroidStar_Portable.app -dmg -codesign="$CODESIGN_ID"
    else
        macdeployqt DroidStar_Portable.app -dmg
    fi
    
    if [ -f "DroidStar_Portable.dmg" ]; then
        DMG_SIZE=$(du -sh DroidStar_Portable.dmg | awk '{print $1}')
        echo "✅ DMG created successfully: DroidStar_Portable.dmg ($DMG_SIZE)"
    else
        echo "❌ Error: DMG creation failed"
        exit 1
    fi
fi

echo ""
echo "🎉 Deployment complete!"
echo ""
echo "📁 Output files:"
echo "   • DroidStar_Portable.app - Portable application bundle ($APP_SIZE)"
if [ "$CREATE_DMG" = true ] && [ -f "DroidStar_Portable.dmg" ]; then
    echo "   • DroidStar_Portable.dmg - Distribution disk image ($DMG_SIZE)"
fi
echo ""
echo "🚀 To test the portable app:"
echo "   open DroidStar_Portable.app"
echo ""
if [ "$CREATE_DMG" = true ]; then
    echo "📦 To distribute:"
    echo "   Share DroidStar_Portable.dmg - users can mount and drag to Applications"
fi
echo ""