#
# Copyright (c) 2025 Lukas Karas <lukas.karas@centrum.cz>
#
# Structure of Camera Buddy cmake scripts was inspired by TimeLapse Tools project
#
# Redistribution and use is allowed according to the terms of the BSD license.

cmake_policy(SET CMP0048 NEW)
project(harbour-camera-buddy VERSION 0.2.0 LANGUAGES C CXX)

# project version
set(CAMERA_BUDDY_MAJOR_VERSION ${PROJECT_VERSION_MAJOR})
set(CAMERA_BUDDY_MINOR_VERSION ${PROJECT_VERSION_MINOR})
set(CAMERA_BUDDY_PATCH_VERSION ${PROJECT_VERSION_PATCH})

set(CAMERA_BUDDY_SUFFIX_VERSION "")
set(CAMERA_BUDDY_VERSION_SHORT  "${CAMERA_BUDDY_MAJOR_VERSION}.${CAMERA_BUDDY_MINOR_VERSION}.${CAMERA_BUDDY_PATCH_VERSION}")
set(CAMERA_BUDDY_VERSION_STRING "${CAMERA_BUDDY_VERSION_SHORT}${CAMERA_BUDDY_SUFFIX_VERSION}")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/CMakeMod/Version.h.cmake"
               "${CMAKE_CURRENT_BINARY_DIR}/privateinclude/harbour-camera-buddy/private/Version.h")

set(CMAKE_MIN_VERSION    "3.9.2") # CMake available on Travis CI is 3.9.2
set(QT_MIN_VERSION       "5.6.0") # QList::constBegin was introduced in Qt 5.6

cmake_minimum_required(VERSION ${CMAKE_MIN_VERSION})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_HOME_DIRECTORY}/CMakeMod")

# Local macros
include(MacroUtils)

include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)
include(CheckPrototypeDefinition)
include(CheckCCompilerFlag)
include(CheckTypeSize)
include(CheckFunctionExists)
include(ExternalProject)

# ==================================================================================================
# Find dependencies

find_package( Qt5 ${QT_MIN_VERSION}
              REQUIRED NO_MODULE COMPONENTS
              Core
              DBus
              Gui
              LinguistTools
              Multimedia
              Qml
              Quick
              Svg
)
find_package(Qt5LinguistTools)
find_package(SailfishApp) # https://github.com/sailfish-sdk/libsailfishapp

add_subdirectory(dependencies)

# ==================================================================================================
# Compiler flags

# see https://doc.qt.io/qtcreator/creator-debugging-qml.html for more details
option(QT_QML_DEBUG "Build with QML debugger support" OFF)
if (QT_QML_DEBUG)
    add_definitions( -DQT_QML_DEBUG)
else()
    set(QT_QML_DEBUG FALSE)
endif()

option(SANITIZER "Build with sanitizer" none)
if(SANITIZER STREQUAL "address")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
elseif(SANITIZER STREQUAL "undefined")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
elseif(SANITIZER STREQUAL "thread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
else()
    set(SANITIZER "none")
endif()

string(TOLOWER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_LOWER)
if (NOT BUILD_TYPE_LOWER STREQUAL "debug")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT result OUTPUT output)
    if(result)
        set(CAMERA_BUDDY_ENABLE_IPO TRUE)
    else()
        message(WARNING "IPO is not supported")
        set(CAMERA_BUDDY_ENABLE_IPO FALSE)
    endif()
else()
    set(CAMERA_BUDDY_ENABLE_IPO FALSE)
endif()

# global settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON) # build should fail when compiler don't support standard defined by CMAKE_CXX_STANDARD
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_VERBOSE_MAKEFILE OFF)

add_definitions( -Wall -Wextra -pedantic -Wno-unused-function )
add_definitions( -fno-omit-frame-pointer -fstack-protector-all -fPIC )

# Do *NOT* turn assertions off in release and rel-with-deb-info modes.
# CMakes does that by default despite any recent documentation
# recommending against the practice.
string( REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
string( REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
string( REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
string( REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

# add function name to Qt log messages (even in release build)
add_definitions( -DQT_MESSAGELOGCONTEXT )

add_definitions(${LIBSAILFISHAPP_DEFINITIONS})

# CMake doesn't add "-pie" by default for executables (CMake issue #14983)
# it is necessary for accelerated start by mapplauncherd,
# see https://git.sailfishos.org/mer-core/mapplauncherd for details
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie")

set(HARBOUR_APP_NAME harbour-camera-buddy)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# ==================================================================================================
# Build executable

set(HEADER_FILES
    src/CameraBuddy.h
    src/CameraModel.h
    src/PhotosModel.h
    src/Settings.h
)

set(SOURCE_FILES
    src/CameraBuddy.cpp
    src/PhotosModel.cpp
    src/CameraModel.cpp
    src/Settings.cpp
    src/DownloadModel.cpp
)

# keep qml files in source list - it makes qtcreator happy
# find qml -type f
set(QML_FILES
    qml/main.qml
    qml/pages/CamerasPage.qml
    qml/pages/PhotosPage.qml
    qml/pages/SettingsPage.qml
    qml/pages/DownloadPage.qml
    qml/pages/Cover.qml
)
add_executable(harbour-camera-buddy
    ${SOURCE_FILES}
    ${HEADER_FILES}
    ${QML_FILES}
)

target_include_directories(harbour-camera-buddy PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/privateinclude
    src
)

# allow exporting main symbol for app booster
# it avoid sfdk check error:
# ERROR [/usr/bin/harbour-camera-buddy] Binary must export main() symbol for booster to work (Q_DECL_EXPORT)
set_target_properties(harbour-camera-buddy PROPERTIES ENABLE_EXPORTS ON)

target_link_libraries(harbour-camera-buddy
    Qt5::Core
    Qt5::DBus
    Qt5::Gui
    Qt5::Multimedia
    Qt5::Qml
    Qt5::Quick
    Qt5::Svg

    ${LIBSAILFISHAPP_LIBRARIES}
    GPhoto2::GPhoto2
)

if(CAMERA_BUDDY_ENABLE_IPO)
    set_property(TARGET harbour-camera-buddy PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

add_dependencies(harbour-camera-buddy translations)

# ==================================================================================================
# Translations

# XML files with translated phrases.
# You can add new language translation just by adding new entry here, and run build.
# Newly created *.ts file may be edited by linguist tool.
set(TRANSLATION_TS_FILES
    translations/en.ts

    translations/cs.ts
)

# source files that may contains translatable phrases (see tr, qsTr, QT_TRANSLATE_NOOP functions)
set(TRANSLATION_SOURCE_FILES
    ${SOURCE_FILES}
    ${QML_FILES}
)

# See http://doc.qt.io/qt-5/cmake-manual.html
#
# qt5_create_translation macro process translations in two steps:
#
#  - collect available phrases inside TRANSLATION_SOURCE_FILES
#    with lupdate tool and put them into *.ts files if some is missing there.
#
#  - create binary translation files (*.qm) from translated phrases available
#    in *.ts files with lrelease tool
qt5_create_translation(TRANSLATION_QM_FILES
    ${TRANSLATION_SOURCE_FILES}
    ${TRANSLATION_TS_FILES})

add_custom_target(translations ALL DEPENDS ${TRANSLATION_QM_FILES})


# ==================================================================================================
# Install

install(TARGETS harbour-camera-buddy
    RUNTIME DESTINATION bin
)

install(FILES harbour-camera-buddy.desktop
    DESTINATION share/applications
)

install(FILES ${TRANSLATION_QM_FILES}
    DESTINATION share/harbour-camera-buddy/translations
)

install(DIRECTORY qml/
    DESTINATION share/harbour-camera-buddy/qml
)

install(FILES icons/86x86/harbour-camera-buddy.png
    DESTINATION share/icons/hicolor/86x86/apps)

install(FILES icons/108x108/harbour-camera-buddy.png
    DESTINATION share/icons/hicolor/108x108/apps)

install(FILES icons/128x128/harbour-camera-buddy.png
    DESTINATION share/icons/hicolor/128x128/apps)

install(FILES icons/256x256/harbour-camera-buddy.png
    DESTINATION share/icons/hicolor/256x256/apps)

install(FILES icons/172x172/harbour-camera-buddy.png
    DESTINATION share/icons/hicolor/172x172/apps)

install(FILES ${DEP_LIBS}
    DESTINATION share/harbour-camera-buddy/lib
)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/dependencies/installroot/lib/libgphoto2_port
    DESTINATION share/${HARBOUR_APP_NAME}/lib
    FILES_MATCHING PATTERN "*.so")

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/dependencies/installroot/lib/libgphoto2/2.5.29
    DESTINATION share/${HARBOUR_APP_NAME}/lib/libgphoto2
    FILES_MATCHING PATTERN "*.so")
