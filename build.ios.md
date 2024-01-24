# Prepare QT environment
```
pip3 install aqtinstall 

aqt install-qt mac desktop 6.6.1 clang_64
aqt install-qt mac ios 6.6.1     

#aqt list-qt mac ios --modules 6.6.1     ios
aqt install-qt mac ios 6.6.1 -m qtmultimedia

./6.6.1/ios/bin/qmake ../DroidStar.pro
make

open DroidStar.xcodeproj
```




# References
- https://stackoverflow.com/questions/2664885/xcode-build-and-archive-from-command-line
- https://github.com/marketplace/actions/install-qt
