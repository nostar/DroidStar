cmake_minimum_required(VERSION 3.16)
project(DroidStar VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(IOS)
    find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Multimedia Network Quick QuickControls2)
else()
    find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Multimedia Network Quick QuickControls2 SerialPort)
endif()

qt_standard_project_setup(REQUIRES 6.5)

set(app_icon_resource_windows "${CMAKE_CURRENT_SOURCE_DIR}/droidstar.rc")
set(MACOSX_BUNDLE_ICON_FILE droidstar.icns)
set(app_icon_macos "${CMAKE_CURRENT_SOURCE_DIR}/images/droidstar.icns")
set_source_files_properties(${app_icon_macos} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

qt_add_executable(DroidStar WIN32 MACOSX_BUNDLE
    CRCenc.cpp CRCenc.h
    DMRDefines.h
    Golay24128.cpp Golay24128.h
    M17Convolution.cpp M17Convolution.h
    M17Defines.h
    MMDVMDefines.h
    SHA256.cpp SHA256.h
    YSFConvolution.cpp YSFConvolution.h
    YSFFICH.cpp YSFFICH.h
    audioengine.cpp audioengine.h
    cbptc19696.cpp cbptc19696.h
    cgolay2087.cpp cgolay2087.h
    chamming.cpp chamming.h
    crs129.cpp crs129.h
    dcs.cpp dcs.h
    dmr.cpp dmr.h
    droidstar.cpp droidstar.h
    globalhotkey.cpp globalhotkey.h
    midihotkey.cpp midihotkey.h
    httpmanager.cpp httpmanager.h
    iax.cpp iax.h
    iaxdefines.h
    imbe_vocoder/aux_sub.cc imbe_vocoder/aux_sub.h
    imbe_vocoder/basic_op.h
    imbe_vocoder/basicop2.cc
    imbe_vocoder/ch_decode.cc imbe_vocoder/ch_decode.h
    imbe_vocoder/ch_encode.cc imbe_vocoder/ch_encode.h
    imbe_vocoder/dc_rmv.cc imbe_vocoder/dc_rmv.h
    imbe_vocoder/decode.cc imbe_vocoder/decode.h
    imbe_vocoder/dsp_sub.cc imbe_vocoder/dsp_sub.h
    imbe_vocoder/encode.cc imbe_vocoder/encode.h
    imbe_vocoder/globals.h
    imbe_vocoder/imbe.h
    imbe_vocoder/imbe_vocoder.cc imbe_vocoder/imbe_vocoder.h
    imbe_vocoder/imbe_vocoder_api.h
    imbe_vocoder/imbe_vocoder_impl.cc imbe_vocoder/imbe_vocoder_impl.h
    imbe_vocoder/math_sub.cc imbe_vocoder/math_sub.h
    imbe_vocoder/pe_lpf.cc imbe_vocoder/pe_lpf.h
    imbe_vocoder/pitch_est.cc imbe_vocoder/pitch_est.h
    imbe_vocoder/pitch_ref.cc imbe_vocoder/pitch_ref.h
    imbe_vocoder/qnt_sub.cc imbe_vocoder/qnt_sub.h
    imbe_vocoder/rand_gen.cc imbe_vocoder/rand_gen.h
    imbe_vocoder/sa_decode.cc imbe_vocoder/sa_decode.h
    imbe_vocoder/sa_encode.cc imbe_vocoder/sa_encode.h
    imbe_vocoder/sa_enh.cc imbe_vocoder/sa_enh.h
    imbe_vocoder/tbls.cc imbe_vocoder/tbls.h
    imbe_vocoder/typedef.h
    imbe_vocoder/typedefs.h
    imbe_vocoder/uv_synt.cc imbe_vocoder/uv_synt.h
    imbe_vocoder/v_synt.cc imbe_vocoder/v_synt.h
    imbe_vocoder/v_uv_det.cc imbe_vocoder/v_uv_det.h
    m17.cpp m17.h
    main.cpp
    mode.cpp mode.h
    nxdn.cpp nxdn.h
    p25.cpp p25.h
    ref.cpp ref.h
    vocoder_plugin.h
    xrf.cpp xrf.h
    ysf.cpp ysf.h
    ${app_icon_resource_windows}
    ${app_icon_macos}
)

qt_add_qml_module(DroidStar
    URI DroidStarApp
    VERSION 1.0
    QML_FILES Main.qml
    QML_FILES MainTab.qml
    QML_FILES SettingsTab.qml
    QML_FILES LogTab.qml
    QML_FILES HostsTab.qml
    QML_FILES AboutTab.qml
)

execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    OUTPUT_VARIABLE VERSION_NUMBER
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Check for RtMidi support
option(ENABLE_MIDI "Enable MIDI hotkey support" ON)

if(ENABLE_MIDI)
    set(MIDI_FOUND FALSE)
    
    # Try to find RtMidi using pkg-config first (preferred method)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(RTMIDI rtmidi)
        if(RTMIDI_FOUND)
            message(STATUS "Found RtMidi via pkg-config: ${RTMIDI_VERSION}")
            target_compile_definitions(DroidStar PRIVATE ENABLE_MIDI)
            target_include_directories(DroidStar PRIVATE ${RTMIDI_INCLUDE_DIRS})
            target_link_libraries(DroidStar PRIVATE ${RTMIDI_LIBRARIES})
            target_link_directories(DroidStar PRIVATE ${RTMIDI_LIBRARY_DIRS})
            set(MIDI_FOUND TRUE)
        endif()
    endif()
    
    # If pkg-config didn't work, try platform-specific detection
    if(NOT MIDI_FOUND)
        # Try to find RtMidi header
        find_path(RTMIDI_INCLUDE_DIR 
            NAMES RtMidi.h rtmidi/RtMidi.h
            PATHS 
                /usr/include
                /usr/local/include
                /opt/homebrew/include
                $ENV{RTMIDI_DIR}/include
        )
        
        # Try to find RtMidi library
        find_library(RTMIDI_LIBRARY
            NAMES rtmidi
            PATHS
                /usr/lib
                /usr/local/lib
                /opt/homebrew/lib
                $ENV{RTMIDI_DIR}/lib
        )
        
        if(RTMIDI_INCLUDE_DIR AND RTMIDI_LIBRARY)
            message(STATUS "Found RtMidi: ${RTMIDI_LIBRARY}")
            target_compile_definitions(DroidStar PRIVATE ENABLE_MIDI)
            target_include_directories(DroidStar PRIVATE ${RTMIDI_INCLUDE_DIR})
            target_link_libraries(DroidStar PRIVATE ${RTMIDI_LIBRARY})
            set(MIDI_FOUND TRUE)
            
            # Add platform-specific libraries and definitions
            if(WIN32)
                # Windows: Add Windows Multimedia library
                target_link_libraries(DroidStar PRIVATE winmm)
                target_compile_definitions(DroidStar PRIVATE __WINDOWS_MM__)
                message(STATUS "MIDI: Added Windows Multimedia support")
                
            elseif(UNIX AND NOT APPLE)
                # Linux: Check for ALSA and JACK
                find_package(ALSA QUIET)
                if(ALSA_FOUND)
                    target_link_libraries(DroidStar PRIVATE ${ALSA_LIBRARIES})
                    target_include_directories(DroidStar PRIVATE ${ALSA_INCLUDE_DIRS})
                    target_compile_definitions(DroidStar PRIVATE __LINUX_ALSA__)
                    message(STATUS "MIDI: Added ALSA support")
                endif()
                
                # Check for JACK
                find_library(JACK_LIBRARY NAMES jack)
                if(JACK_LIBRARY)
                    target_link_libraries(DroidStar PRIVATE ${JACK_LIBRARY})
                    target_compile_definitions(DroidStar PRIVATE __UNIX_JACK__)
                    message(STATUS "MIDI: Added JACK support")
                endif()
                
                # Always add pthread for Linux
                find_package(Threads REQUIRED)
                target_link_libraries(DroidStar PRIVATE Threads::Threads)
                
            elseif(APPLE)
                # macOS: Add CoreMIDI framework (already handled by RtMidi)
                target_compile_definitions(DroidStar PRIVATE __MACOSX_CORE__)
                message(STATUS "MIDI: Added CoreMIDI support")
            endif()
        endif()
    endif()
    
    if(NOT MIDI_FOUND)
        message(WARNING "RtMidi not found. MIDI support will be disabled.")
        message(STATUS "To enable MIDI support:")
        if(WIN32)
            message(STATUS "  Windows: Install RtMidi via vcpkg or build from source")
        elseif(UNIX AND NOT APPLE)
            message(STATUS "  Linux: sudo apt-get install librtmidi-dev (Ubuntu/Debian)")
            message(STATUS "         sudo dnf install rtmidi-devel (Fedora)")
            message(STATUS "         sudo pacman -S rtmidi (Arch)")
        elseif(APPLE)
            message(STATUS "  macOS: brew install rtmidi")
        endif()
    endif()
endif()

target_compile_definitions(DroidStar PRIVATE
    QT_DEPRECATED_WARNINGS
    VERSION_NUMBER="${VERSION_NUMBER}"
)

target_link_libraries(DroidStar PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Multimedia
    Qt::Network
    Qt::Quick
    Qt::QuickControls2
)

if(UNIX AND NOT IOS)
    target_link_libraries(DroidStar PRIVATE
        Qt::SerialPort
    )
    
    # Add X11 libraries for Linux global hotkeys
    if(NOT APPLE)
        find_package(X11 QUIET)
        if(X11_FOUND)
            target_link_libraries(DroidStar PRIVATE ${X11_LIBRARIES})
            target_include_directories(DroidStar PRIVATE ${X11_INCLUDE_DIR})
            message(STATUS "Found X11 libraries for global hotkeys")
        else()
            message(WARNING "X11 libraries not found. Global hotkeys may not work on Linux.")
        endif()
    endif()
endif()

if(WIN32)
    target_link_libraries(DroidStar PRIVATE
        Qt::SerialPort
        ws2_32
    )
    #add_compile_options(-static --enable-stdcall-fixup)
endif()

if(UNIX)
    target_link_libraries(DroidStar PRIVATE
        dl
    )
endif()

if(APPLE)
    set_target_properties(DroidStar PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)
    target_link_libraries(DroidStar PRIVATE
        "-framework AVFoundation"
        "-framework Carbon"
    )
endif()

if(ANDROID)
    include(~/Android/Sdk/android_openssl/android_openssl.cmake)
    add_android_openssl_libraries(DroidStar)
    target_sources(DroidStar PUBLIC
        androidserialport.cpp androidserialport.h
    )

    set_target_properties(DroidStar PROPERTIES
       QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android
       QT_ANDROID_VERSION_CODE 88
       #QT_ANDROID_MIN_SDK_VERSION 31
    )
endif()

if(NOT IOS)
    target_sources(DroidStar PUBLIC
        serialambe.cpp serialambe.h
        serialmodem.cpp serialmodem.h
    )
endif()

if(FALSE) #set TRUE to use external codec2
    target_compile_definitions(DroidStar PRIVATE
        USE_EXTERNAL_CODEC2
    )
    target_link_libraries(DroidStar PRIVATE
        codec2
    )
else()
    target_sources(DroidStar PUBLIC
        codec2/codebooks.cpp
        codec2/codec2.cpp
        codec2/codec2_api.h
        codec2/codec2_internal.h
        codec2/defines.h
        codec2/kiss_fft.cpp codec2/kiss_fft.h
        codec2/lpc.cpp codec2/lpc.h
        codec2/nlp.cpp codec2/nlp.h
        codec2/pack.cpp
        codec2/qbase.cpp codec2/qbase.h
        codec2/quantise.cpp codec2/quantise.h
    )
endif()

if(FALSE) #set TRUE to use vocoder plugin
    target_compile_definitions(DroidStar PRIVATE
        VOCODER_PLUGIN
    )
else()
    target_sources(DroidStar PUBLIC
        mbe/ambe3600x2400.c
        mbe/ambe3600x2400_const.h
        mbe/ambe3600x2450.c
        mbe/ambe3600x2450_const.h
        mbe/ecc.c
        mbe/ecc_const.h
        mbe/mbelib.c mbe/mbelib.h
        mbe/mbelib_const.h
        mbe/mbelib_parms.h
        mbe/vocoder_plugin.cpp mbe/vocoder_plugin.h
        mbe/vocoder_plugin_api.h
        mbe/vocoder_tables.h
    )
endif()


if(FALSE) # set TRUE for flite
    target_compile_definitions(DroidStar PRIVATE
        USE_FLITE
    )
    target_link_libraries(DroidStar PRIVATE
        asound
        flite_cmu_us_awb
        flite_cmu_us_kal16
        flite_cmu_us_rms
        flite_cmu_us_slt
        flite_cmulex
        flite_usenglish
        flite
    )
endif()

if(FALSE) # set TRUE for md380_vocoder
    target_compile_definitions(DroidStar PRIVATE
        USE_MD380_VOCODER
    )
    target_link_libraries(DroidStar PRIVATE
        md380_vocoder
        #/home/nostar/Android/Sdk/local/libmd380_vocoder.a
        # -Xlinker --section-start=.firmware=0x0800C000 -Xlinker --section-start=.sram=0x20000000
    )
endif()

install(TARGETS DroidStar
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_app_script(
    TARGET DroidStar
    FILENAME_VARIABLE deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
