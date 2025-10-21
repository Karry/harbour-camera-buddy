# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/karry/SailfishOS/projects/harbour-camera-buddy/cmake-build-debug/dependencies/libltdl-prefix/src/libltdl")
  file(MAKE_DIRECTORY "/home/karry/SailfishOS/projects/harbour-camera-buddy/cmake-build-debug/dependencies/libltdl-prefix/src/libltdl")
endif()
file(MAKE_DIRECTORY
  "/home/karry/SailfishOS/projects/harbour-camera-buddy/cmake-build-debug/dependencies/libltdl-prefix/src/libltdl-build"
  "/home/karry/SailfishOS/projects/harbour-camera-buddy/cmake-build-debug/dependencies/installroot"
  "/home/karry/SailfishOS/projects/harbour-camera-buddy/cmake-build-debug/dependencies/libltdl-prefix/tmp"
  "/home/karry/SailfishOS/projects/harbour-camera-buddy/cmake-build-debug/dependencies/libltdl-prefix/src/libltdl-stamp"
  "/home/karry/SailfishOS/projects/harbour-camera-buddy/cmake-build-debug/dependencies/libltdl-prefix/src"
  "/home/karry/SailfishOS/projects/harbour-camera-buddy/cmake-build-debug/dependencies/libltdl-prefix/src/libltdl-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/karry/SailfishOS/projects/harbour-camera-buddy/cmake-build-debug/dependencies/libltdl-prefix/src/libltdl-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/karry/SailfishOS/projects/harbour-camera-buddy/cmake-build-debug/dependencies/libltdl-prefix/src/libltdl-stamp${cfgdir}") # cfgdir has leading slash
endif()
