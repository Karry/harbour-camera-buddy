# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

if("/home/karry/SailfishOS/projects/harbour-camera-buddy/dependencies/libusb-1.0.26.tar.bz2" STREQUAL "")
  message(FATAL_ERROR "LOCAL can't be empty")
endif()

if(NOT EXISTS "/home/karry/SailfishOS/projects/harbour-camera-buddy/dependencies/libusb-1.0.26.tar.bz2")
  message(FATAL_ERROR "File not found: /home/karry/SailfishOS/projects/harbour-camera-buddy/dependencies/libusb-1.0.26.tar.bz2")
endif()

if("SHA256" STREQUAL "")
  message(WARNING "File cannot be verified since no URL_HASH specified")
  return()
endif()

if("12ce7a61fc9854d1d2a1ffe095f7b5fac19ddba095c259e6067a46500381b5a5" STREQUAL "")
  message(FATAL_ERROR "EXPECT_VALUE can't be empty")
endif()

message(VERBOSE "verifying file...
     file='/home/karry/SailfishOS/projects/harbour-camera-buddy/dependencies/libusb-1.0.26.tar.bz2'")

file("SHA256" "/home/karry/SailfishOS/projects/harbour-camera-buddy/dependencies/libusb-1.0.26.tar.bz2" actual_value)

if(NOT "${actual_value}" STREQUAL "12ce7a61fc9854d1d2a1ffe095f7b5fac19ddba095c259e6067a46500381b5a5")
  message(FATAL_ERROR "error: SHA256 hash of
  /home/karry/SailfishOS/projects/harbour-camera-buddy/dependencies/libusb-1.0.26.tar.bz2
does not match expected value
  expected: '12ce7a61fc9854d1d2a1ffe095f7b5fac19ddba095c259e6067a46500381b5a5'
    actual: '${actual_value}'
")
endif()

message(VERBOSE "verifying file... done")
