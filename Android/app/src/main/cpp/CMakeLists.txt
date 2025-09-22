# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)

# Declares the project name. The project name can be accessed via ${ PROJECT_NAME},
# Since this is the top level CMakeLists.txt, the project name is also accessible
# with ${CMAKE_PROJECT_NAME} (both CMake variables are in-sync within the top level
# build script scope).
project("retrogbm")

# Define a base source directory
set(SourceDir "../../../../../")

# Use the base to define your core paths
set(EmulatorCoreIncDir "${SourceDir}Core/Include")
set(EmulatorCoreSrcDir "${SourceDir}Core/Source")
set(EmulatorCoreExternalIncDir "${SourceDir}Core/External")

include_directories(${EmulatorCoreIncDir})
include_directories(${EmulatorCoreExternalIncDir})

# -------- Optimization Flags (aggressive) --------
# Apply only in Release mode
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -flto -ffunction-sections -fdata-sections -fstrict-aliasing -fomit-frame-pointer")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -Wl,--gc-sections")
endif()

# Optional: force C++20 if needed for performance/STL optimizations
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.
#
# In this top level CMakeLists.txt, ${CMAKE_PROJECT_NAME} is used to define
# the target library name; in the sub-module's CMakeLists.txt, ${PROJECT_NAME}
# is preferred for the same purpose.
#
# In order to load a library into your app from Java/Kotlin, you must call
# System.loadLibrary() and pass the name of the library defined here;
# for GameActivity/NativeActivity derived applications, the same library name must be
# used in the AndroidManifest.xml file.
add_library(${CMAKE_PROJECT_NAME} SHARED
        # List C/C++ source files with relative paths to this CMakeLists.txt.
        native-lib.cpp
        KotlinSoundOutputWrapper.cpp
        KotlinDisplayOutputWrapper.cpp
        JniLoader.cpp

        ${EmulatorCoreSrcDir}/Cartridge/BaseCartridge.cpp
        ${EmulatorCoreSrcDir}/Cartridge/CartridgeROM.cpp
        ${EmulatorCoreSrcDir}/Cartridge/CartridgeMBC1.cpp
        ${EmulatorCoreSrcDir}/Cartridge/CartridgeMBC3.cpp
        ${EmulatorCoreSrcDir}/Cartridge/CartridgeMBC5.cpp
        ${EmulatorCoreSrcDir}/Cpu.cpp
        ${EmulatorCoreSrcDir}/Display.cpp
        ${EmulatorCoreSrcDir}/Dma.cpp
        ${EmulatorCoreSrcDir}/Emulator.cpp
        ${EmulatorCoreSrcDir}/ExtendedInstructions.cpp
        ${EmulatorCoreSrcDir}/Instructions.cpp
        ${EmulatorCoreSrcDir}/Joypad.cpp
        ${EmulatorCoreSrcDir}/Ppu.cpp
        ${EmulatorCoreSrcDir}/Ram.cpp
        ${EmulatorCoreSrcDir}/Timer.cpp
        ${EmulatorCoreSrcDir}/HighTimer.cpp
        ${EmulatorCoreSrcDir}/Apu.cpp
        ${EmulatorCoreSrcDir}/Logger.cpp
        ${EmulatorCoreSrcDir}/Cheats.cpp

        ${EmulatorCoreSrcDir}/Audio/FrequencySweep.cpp
        ${EmulatorCoreSrcDir}/Audio/LengthCounter.cpp
        ${EmulatorCoreSrcDir}/Audio/Lfsr.cpp
        ${EmulatorCoreSrcDir}/Audio/PolynomialCounter.cpp
        ${EmulatorCoreSrcDir}/Audio/SoundMode1.cpp
        ${EmulatorCoreSrcDir}/Audio/SoundMode2.cpp
        ${EmulatorCoreSrcDir}/Audio/SoundMode3.cpp
        ${EmulatorCoreSrcDir}/Audio/SoundMode4.cpp
        ${EmulatorCoreSrcDir}/Audio/SoundModeBase.cpp
        ${EmulatorCoreSrcDir}/Audio/VolumeEnvelope.cpp
)

# Specifies libraries CMake should link to your target library. You
# can link libraries from various origins, such as libraries defined in this
# build script, prebuilt third-party libraries, or Android system libraries.
target_link_libraries(${CMAKE_PROJECT_NAME}
        # List libraries link to the target library
        android
        log)

# Force 16 KB page size
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE "-Wl,-z,max-page-size=16384")
