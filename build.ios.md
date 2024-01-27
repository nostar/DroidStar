# Prepare QT environment
```
pip3 install aqtinstall 
QT_VERSION=6.6.1

mkdir build_ios
cd build_ios

aqt install-qt mac desktop $QT_VERSION clang_64
aqt install-qt mac ios $QT_VERSION     
aqt install-qt mac ios $QT_VERSION -m qtmultimedia

./$QT_VERSION/ios/bin/qmake ../DroidStar.pro
make

open DroidStar.xcodeproj
```


# Configration
In Settings
- Callsign
- DMRID
- BM Pass (hotspot)
In Main
- Mode DMR
- Host
- SWTX, SWRX, AGC
- TG: i.e. 9990 with private (parrot)


# References
- https://stackoverflow.com/questions/2664885/xcode-build-and-archive-from-command-line
- https://github.com/marketplace/actions/install-qt
