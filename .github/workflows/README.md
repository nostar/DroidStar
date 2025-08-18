# GitHub Workflows for DroidStar

This directory contains GitHub Actions workflows for building and distributing DroidStar across different platforms.

## Available Workflows

### 📱 `build-macos-portable.yml` - macOS Portable Binary Builder

Creates universal macOS application bundles that can be distributed without requiring users to install dependencies.

#### Features
- **Universal Binary**: Builds for both ARM64 (Apple Silicon) and x86_64 (Intel) architectures
- **ARM Priority**: Optimized for Apple Silicon Macs with Intel compatibility
- **Portable Distribution**: All Qt frameworks and dependencies bundled within app
- **Manual Triggering**: Run on-demand with branch selection
- **Code Signing Support**: Optional code signing for trusted distribution
- **DMG Creation**: Generates compressed disk images for easy distribution

#### Usage

1. **Navigate to Actions tab** in your GitHub repository
2. **Select "Build macOS Portable Binary"** workflow
3. **Click "Run workflow"** and configure:

   | Parameter | Description | Default | Options |
   |-----------|-------------|---------|---------|
   | `branch` | Branch to build from | `main` | Any valid branch name |
   | `build_type` | Build configuration | `Release` | `Release`, `Debug` |
   | `create_dmg` | Create DMG package | `true` | `true`, `false` |
   | `code_sign` | Code sign application | `false` | `true`, `false` |

4. **Click "Run workflow"** to start the build

#### Code Signing Setup (Optional)

For code signing to work, add this secret to your repository:

- **`DEVELOPER_ID_APPLICATION`**: Your Apple Developer ID Application certificate name
  - Example: `"Developer ID Application: Your Name (XXXXXXXXXX)"`
  - Find this in Keychain Access under "My Certificates"

To add the secret:
1. Go to repository **Settings** → **Secrets and variables** → **Actions**
2. Click **"New repository secret"**
3. Name: `DEVELOPER_ID_APPLICATION`
4. Value: Your exact certificate name from Keychain

#### Build Outputs

The workflow generates these artifacts:

| Artifact | Description | Size | Use Case |
|----------|-------------|------|----------|
| `*_app` | Portable app bundle | ~74MB | Direct execution, copy to Applications |
| `*_dmg` | DMG disk image | ~37MB | Distribution, user installation |
| `*_build_summary` | Build information | <1KB | Verification, debugging |

#### Build Process

1. **Environment Setup**
   - macOS latest runner (currently macOS 12+)
   - Qt 6.5.0 with multimedia and serial port modules
   - Homebrew dependencies (cmake, rtmidi, pkg-config)

2. **Universal Binary Configuration**
   - Target architectures: `arm64;x86_64` 
   - Deployment target: macOS 11.0+
   - ARM64 priority for optimal Apple Silicon performance

3. **Dependency Bundling**
   - Qt frameworks copied to app bundle
   - RtMidi library bundled for MIDI support
   - All dynamic libraries resolved and included

4. **Verification**
   - Architecture verification with `lipo`
   - Dependency checking with `otool`
   - Basic functionality testing

#### Troubleshooting

**Common Issues:**

- **Qt Installation Failed**: Workflow will retry Qt installation automatically
- **Code Signing Failed**: Verify `DEVELOPER_ID_APPLICATION` secret is correct
- **Build Failed**: Check build logs in the workflow run details
- **DMG Creation Failed**: Usually due to insufficient disk space or permissions

**Debug Steps:**

1. **Check workflow logs** for detailed error messages
2. **Download build summary** artifact for dependency and architecture info  
3. **Try without code signing** first to isolate issues
4. **Test with Debug build** for additional diagnostic information

#### Customization

To modify the workflow:

1. **Change Qt Version**: Update `QT_VERSION` environment variable
2. **Modify Architecture**: Update `CMAKE_OSX_ARCHITECTURES` 
3. **Add Dependencies**: Add to Homebrew install step
4. **Customize Deployment**: Modify `macdeployqt` parameters

#### Manual Local Build

To replicate the workflow locally:

```bash
# Install dependencies
brew install cmake qt rtmidi pkg-config

# Configure universal build
export CMAKE_OSX_ARCHITECTURES="arm64;x86_64"
export CMAKE_OSX_DEPLOYMENT_TARGET="11.0"

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_MIDI=ON
make -j$(sysctl -n hw.ncpu)

# Create portable app
cp -r DroidStar.app DroidStar_Portable.app
macdeployqt DroidStar_Portable.app -dmg
```

### 🪟 `build-windows-portable.yml` - Windows Portable Executable Builder

Creates portable Windows executables with all dependencies bundled, plus optional NSIS installer.

#### Features
- **Multi-Architecture**: Supports both x64 and x86 builds
- **Portable Distribution**: All Qt frameworks and dependencies bundled
- **NSIS Installer**: Optional professional installer with Start Menu integration
- **MIDI Support**: Full RtMidi integration via vcpkg
- **Manual Triggering**: Run on-demand with branch and architecture selection

#### Usage

1. **Navigate to Actions tab** in your GitHub repository
2. **Select "Build Windows Portable Executable"** workflow
3. **Click "Run workflow"** and configure:

   | Parameter | Description | Default | Options |
   |-----------|-------------|---------|---------|
   | `branch` | Branch to build from | `main` | Any valid branch name |
   | `build_type` | Build configuration | `Release` | `Release`, `Debug` |
   | `create_installer` | Create NSIS installer | `true` | `true`, `false` |
   | `architecture` | Target architecture | `x64` | `x64`, `x86` |

4. **Click "Run workflow"** to start the build

#### Build Outputs

The workflow generates these artifacts:

| Artifact | Description | Size | Use Case |
|----------|-------------|------|----------|
| `*_portable` | Portable executable bundle | ~100MB | Direct execution, no installation needed |
| `*_installer` | NSIS installer (.exe) | ~45MB | Full Windows integration, Start Menu shortcuts |
| `*_build_summary` | Build information | <1KB | Verification, debugging |

#### Build Process

1. **Environment Setup**
   - Windows latest runner (Windows Server 2022)
   - Qt 6.5.0 with multimedia and serial port modules
   - MSVC 2019 compiler with selected architecture
   - vcpkg for dependency management

2. **Dependency Management**
   - RtMidi installed via vcpkg for MIDI support
   - Automatic Qt deployment with windeployqt
   - All DLLs bundled for portable operation

3. **NSIS Installer Creation**
   - Professional Windows installer
   - Start Menu and Desktop shortcuts
   - Proper uninstall support
   - Registry integration for Add/Remove Programs

4. **Verification**
   - Executable functionality testing
   - Dependency completeness check
   - Bundle portability validation

#### Troubleshooting Windows Builds

**Common Issues:**

- **vcpkg Installation Failed**: Network issues or GitHub cache problems
- **Qt Deployment Failed**: Missing windeployqt or incorrect Qt installation
- **NSIS Installer Failed**: NSIS download or compilation issues
- **Runtime Errors**: Missing Visual C++ redistributables

**Debug Steps:**

1. **Check workflow logs** for detailed error messages
2. **Download build summary** for dependency information
3. **Try Debug build** for additional diagnostic information
4. **Test without installer** to isolate NSIS issues

## Future Workflows

Planned workflows for other platforms:

- `build-linux-appimage.yml` - Linux AppImage for universal Linux distribution
- `build-android-apk.yml` - Android APK for mobile devices

## Contributing

When adding new workflows:

1. Follow the existing naming convention: `build-{platform}-{type}.yml`
2. Include comprehensive parameter validation and error handling
3. Generate detailed build summaries and artifacts
4. Add documentation to this README
5. Test thoroughly with different parameter combinations