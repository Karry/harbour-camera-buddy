# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/karry/SailfishOS/projects/harbour-camera-buddy/cmake-build-debug/dependencies/GPhoto2-prefix/src/GPhoto2")
  file(MAKE_DIRECTORY "/home/karry/SailfishOS/projects/harbour-camera-buddy/cmake-build-debug/dependencies/GPhoto2-prefix/src/GPhoto2")
endif()
file(MAKE_DIRECTORY
  "/home/karry/SailfishOS/projects/harbour-camera-buddy/cmake-build-debug/dependencies/GPhoto2-prefix/src/GPhoto2-build"
  "/home/karry/SailfishOS/projects/harbour-camera-buddy/cmake-build-debug/dependencies/installroot"
  "/home/karry/SailfishOS/projects/harbour-camera-buddy/cmake-build-debug/dependencies/GPhoto2-prefix/tmp"
  "/home/karry/SailfishOS/projects/harbour-camera-buddy/cmake-build-debug/dependencies/GPhoto2-prefix/src/GPhoto2-stamp"
  "/home/karry/SailfishOS/projects/harbour-camera-buddy/cmake-build-debug/dependencies/GPhoto2-prefix/src"
  "/home/karry/SailfishOS/projects/harbour-camera-buddy/cmake-build-debug/dependencies/GPhoto2-prefix/src/GPhoto2-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/karry/SailfishOS/projects/harbour-camera-buddy/cmake-build-debug/dependencies/GPhoto2-prefix/src/GPhoto2-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/karry/SailfishOS/projects/harbour-camera-buddy/cmake-build-debug/dependencies/GPhoto2-prefix/src/GPhoto2-stamp${cfgdir}") # cfgdir has leading slash
endif()
