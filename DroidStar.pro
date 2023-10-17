QT += quick quickcontrols2 network multimedia

equals(QT_MAJOR_VERSION, 5){
	android:QT += androidextras
}

unix:!ios:QT += serialport
CONFIG += c++17
LFLAGS +=
!win32:LIBS += -ldl
win32:QT += serialport
#win32:LIBS += -lws2_32
win32:QMAKE_LFLAGS += -static
QMAKE_LFLAGS_WINDOWS += --enable-stdcall-fixup
RC_ICONS = images/droidstar.ico
ICON = images/droidstar.icns
macx:LIBS += -framework AVFoundation
macx:QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.14
macx:QMAKE_INFO_PLIST = Info.plist.mac
ios:LIBS += -framework AVFoundation
ios:QMAKE_IOS_DEPLOYMENT_TARGET=14.0
ios:QMAKE_TARGET_BUNDLE_PREFIX = org.dudetronics
ios:QMAKE_BUNDLE = droidstar
ios:VERSION = 0.43.20
ios:Q_ENABLE_BITCODE.name = ENABLE_BITCODE
ios:Q_ENABLE_BITCODE.value = NO
ios:QMAKE_MAC_XCODE_SETTINGS += Q_ENABLE_BITCODE
ios:QMAKE_ASSET_CATALOGS += Images.xcassets
ios:QMAKE_INFO_PLIST = Info.plist
VERSION_BUILD='$(shell cd $$PWD;git rev-parse --short HEAD)'
DEFINES += VERSION_NUMBER=\"\\\"$${VERSION_BUILD}\\\"\"
DEFINES += QT_DEPRECATED_WARNINGS
#DEFINES += QT_DEBUG_PLUGINS=1
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
#DEFINES += VOCODER_PLUGIN
#DEFINES += USE_FLITE
#DEFINES += USE_EXTERNAL_CODEC2

HEADERS += \
	CRCenc.h \
	DMRDefines.h \
	Golay24128.h \
	M17Convolution.h \
	M17Defines.h \
	MMDVMDefines.h \
	SHA256.h \
	YSFConvolution.h \
	YSFFICH.h \
	audioengine.h \
	cbptc19696.h \
	cgolay2087.h \
	chamming.h \
	crs129.h \
	dcs.h \
	dmr.h \
	droidstar.h \
	httpmanager.h \
	iax.h \
	iaxdefines.h \
	imbe_vocoder/aux_sub.h \
	imbe_vocoder/basic_op.h \
	imbe_vocoder/ch_decode.h \
	imbe_vocoder/ch_encode.h \
	imbe_vocoder/dc_rmv.h \
	imbe_vocoder/decode.h \
	imbe_vocoder/dsp_sub.h \
	imbe_vocoder/encode.h \
	imbe_vocoder/globals.h \
	imbe_vocoder/imbe.h \
	imbe_vocoder/imbe_vocoder.h \
	imbe_vocoder/imbe_vocoder_api.h \
	imbe_vocoder/imbe_vocoder_impl.h \
	imbe_vocoder/math_sub.h \
	imbe_vocoder/pe_lpf.h \
	imbe_vocoder/pitch_est.h \
	imbe_vocoder/pitch_ref.h \
	imbe_vocoder/qnt_sub.h \
	imbe_vocoder/rand_gen.h \
	imbe_vocoder/sa_decode.h \
	imbe_vocoder/sa_encode.h \
	imbe_vocoder/sa_enh.h \
	imbe_vocoder/tbls.h \
	imbe_vocoder/typedef.h \
	imbe_vocoder/typedefs.h \
	imbe_vocoder/uv_synt.h \
	imbe_vocoder/v_synt.h \
	imbe_vocoder/v_uv_det.h \
	m17.h \
	mode.h \
	nxdn.h \
	p25.h \
	ref.h \
	vocoder_plugin.h \
	xrf.h \
	ysf.h
android:HEADERS += androidserialport.h
macx:HEADERS += micpermission.h
!ios:HEADERS += serialambe.h serialmodem.h

SOURCES += \
	CRCenc.cpp \
	Golay24128.cpp \
	M17Convolution.cpp \
	SHA256.cpp \
	YSFConvolution.cpp \
	YSFFICH.cpp \
	audioengine.cpp \
	cbptc19696.cpp \
	cgolay2087.cpp \
	chamming.cpp \
	crs129.cpp \
	dcs.cpp \
	dmr.cpp \
	droidstar.cpp \
	httpmanager.cpp \
	iax.cpp \
	imbe_vocoder/aux_sub.cc \
	imbe_vocoder/basicop2.cc \
	imbe_vocoder/ch_decode.cc \
	imbe_vocoder/ch_encode.cc \
	imbe_vocoder/dc_rmv.cc \
	imbe_vocoder/decode.cc \
	imbe_vocoder/dsp_sub.cc \
	imbe_vocoder/encode.cc \
	imbe_vocoder/imbe_vocoder.cc \
	imbe_vocoder/imbe_vocoder_impl.cc \
	imbe_vocoder/math_sub.cc \
	imbe_vocoder/pe_lpf.cc \
	imbe_vocoder/pitch_est.cc \
	imbe_vocoder/pitch_ref.cc \
	imbe_vocoder/qnt_sub.cc \
	imbe_vocoder/rand_gen.cc \
	imbe_vocoder/sa_decode.cc \
	imbe_vocoder/sa_encode.cc \
	imbe_vocoder/sa_enh.cc \
	imbe_vocoder/tbls.cc \
	imbe_vocoder/uv_synt.cc \
	imbe_vocoder/v_synt.cc \
	imbe_vocoder/v_uv_det.cc \
	m17.cpp \
	main.cpp \
	mode.cpp \
	nxdn.cpp \
	p25.cpp \
	ref.cpp \
	xrf.cpp \
	ysf.cpp
android:SOURCES += androidserialport.cpp
!ios:SOURCES += serialambe.cpp serialmodem.cpp
!contains(DEFINES, USE_EXTERNAL_CODEC2){
HEADERS += \
	codec2/codec2_api.h \
	codec2/codec2_internal.h \
	codec2/defines.h \
	codec2/kiss_fft.h \
	codec2/lpc.h \
	codec2/nlp.h \
	codec2/qbase.h \
	codec2/quantise.h
SOURCES += \
	codec2/codebooks.cpp \
	codec2/codec2.cpp \
	codec2/kiss_fft.cpp \
	codec2/lpc.cpp \
	codec2/nlp.cpp \
	codec2/pack.cpp \
	codec2/qbase.cpp \
	codec2/quantise.cpp
}
contains(DEFINES, USE_EXTERNAL_CODEC2){
LIBS += -lcodec2
}
!contains(DEFINES, VOCODER_PLUGIN){
HEADERS += \
	mbe/ambe3600x2400_const.h \
	mbe/ambe3600x2450_const.h \
	mbe/ecc_const.h \
	mbe/mbelib.h \
	mbe/mbelib_const.h \
	mbe/mbelib_parms.h \
	mbe/vocoder_plugin.h \
	mbe/vocoder_plugin_api.h \
	mbe/vocoder_tables.h
SOURCES += \
	mbe/ambe3600x2400.c \
	mbe/ambe3600x2450.c \
	mbe/ecc.c \
	mbe/mbelib.c \
	mbe/vocoder_plugin.cpp
}

macx:OBJECTIVE_SOURCES += micpermission.mm
ios:OBJECTIVE_SOURCES += micpermission.mm
RESOURCES += qml.qrc

QML_IMPORT_PATH =
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
	android/AndroidManifest.xml \
	android/build.gradle \
	android/gradle/wrapper/gradle-wrapper.jar \
	android/gradle/wrapper/gradle-wrapper.properties \
	android/gradlew \
	android/gradlew.bat \
	android/res/values/libs.xml \
	images/log.png

contains(ANDROID_TARGET_ARCH,armeabi-v7a) {
	LIBS += -L$$(HOME)/Android/local/lib
	ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
	OTHER_FILES += android/src
}

contains(ANDROID_TARGET_ARCH,arm64-v8a) {
	LIBS += -L$$(HOME)/Android/local/lib64
	ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
	OTHER_FILES += android/src
	#ANDROID_ABIS = arm64-v8a
}

#ANDROID_ABIS = armeabi-v7a arm64-v8a

contains(DEFINES, USE_FLITE){
	LIBS += -lflite_cmu_us_slt -lflite_cmu_us_kal16 -lflite_cmu_us_awb -lflite_cmu_us_rms -lflite_usenglish -lflite_cmulex -lflite -lasound
}
ios:HEADERS += micpermission.h
