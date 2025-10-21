#
# Copyright (c) 2024 Camera Buddy
#
# Structure of Camera Buddy cmake scripts was inspired by TimeLapse Tools project
#
# Redistribution and use is allowed according to the terms of the BSD license.

cmake_policy(SET CMP0048 NEW)
project(harbour-camera-buddy VERSION 0.1.0 LANGUAGES C CXX)

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

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# ==================================================================================================
# Build executable

set(CAMERA_BUDDY_HEADERS
    src/CameraBuddy.h
)

set(CAMERA_BUDDY_SOURCES
    src/CameraBuddy.cpp
)

add_executable(harbour-camera-buddy
    ${CAMERA_BUDDY_SOURCES}
    ${CAMERA_BUDDY_HEADERS}
)

target_include_directories(harbour-camera-buddy PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/privateinclude
    src
)

target_link_libraries(harbour-camera-buddy
    Qt5::Core
    Qt5::DBus
    Qt5::Gui
    Qt5::Multimedia
    Qt5::Qml
    Qt5::Quick
    Qt5::Svg
    SailfishApp::SailfishApp
    GPhoto2::GPhoto2
)

if(CAMERA_BUDDY_ENABLE_IPO)
    set_property(TARGET harbour-camera-buddy PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# ==================================================================================================
# Translations

file(GLOB TRANSLATION_SOURCES
    translations/*.ts
)
qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TRANSLATION_SOURCES})

# ==================================================================================================
# Install

install(TARGETS harbour-camera-buddy
    RUNTIME DESTINATION bin
)

install(FILES harbour-camera-buddy.desktop
    DESTINATION share/applications
)

install(FILES ${QM_FILES}
    DESTINATION share/harbour-camera-buddy/translations
)

install(DIRECTORY qml/
    DESTINATION share/harbour-camera-buddy/qml
)

install(DIRECTORY icons/
    DESTINATION share/harbour-camera-buddy/icons
)

install(FILES ${DEP_LIBS}
    DESTINATION share/harbour-camera-buddy/lib
)
