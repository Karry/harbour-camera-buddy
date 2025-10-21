# Instruction for GitHub Copilot

Our goal is to create application for SailfishOS that will be able to connect to external DSLR cameras
via USB or WiFi (using GPhoto2 library) and download photos from them to the device storage. Lets call this application "Camera Buddy".
To comply with SailfishOS requirements, binary and path components needs to be prefixed by keyword "harbour-", 
so the executable binary will be harbour-camera-buddy.

Sa a example project we will use "TimeLapse tools" application, available in docs/harbour-timelapse-tools directory.
This application already integrates GPhoto2 library and is able to take pictures from external cameras.

Camera Buddy will be written in C++ (c++20 standard), using Qt 5.6 framework and UI IN QML language. Build system will be CMake.
Files should be structured in the repository the same way as in "TimeLapse tools" application.
Application will be ready for to rpm packaging for SailfishOS.

## Dependencies

 - GPhoto2 library, see `docs/harbour-timelapse-tools/dependencies/CMakeLists.txt` 
  howto include it in CMake build system.
 - libusb library (dependency of GPhoto2), see `docs/harbour-timelapse-tools/dependencies/CMakeLists.txt`
  howto include it in CMake build system.
 - dependencies included in SailfishOS SDK: 
   - Qt5Core
   - Qt5DBus
   - Qt5Multimedia
   - Qt5Qml
   - Qt5Quick
   - Qt5Svg
   - sailfishapp
   - libcurl
   - libxml

# Compilation

 - For fast verification that build is working, we may build it locally. 
 - After bigger changes, build needs to be verified in SailfishOS SDK environment, `deploy.sh` may be used for that.
 - To be able verify build in Github, there should be github action workflow, at least with ubuntu 24.04.

# UI

Application should be consisting from three pages:
 - Cameras page: list of connected cameras, with refresh button
 - Photos page: shows photos on selected camera with checkbox to select photos to download and download button
   After pressing "download" button, application should open dialog for selecting download directory, and then download selected photos
 - Downloads page: shows progress of current downloads.

