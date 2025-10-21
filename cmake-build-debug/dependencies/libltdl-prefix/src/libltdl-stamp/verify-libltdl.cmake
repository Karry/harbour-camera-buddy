# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

if("/home/karry/SailfishOS/projects/harbour-camera-buddy/dependencies/libtool-2.4.6.tar.gz" STREQUAL "")
  message(FATAL_ERROR "LOCAL can't be empty")
endif()

if(NOT EXISTS "/home/karry/SailfishOS/projects/harbour-camera-buddy/dependencies/libtool-2.4.6.tar.gz")
  message(FATAL_ERROR "File not found: /home/karry/SailfishOS/projects/harbour-camera-buddy/dependencies/libtool-2.4.6.tar.gz")
endif()

if("SHA256" STREQUAL "")
  message(WARNING "File cannot be verified since no URL_HASH specified")
  return()
endif()

if("e3bd4d5d3d025a36c21dd6af7ea818a2afcd4dfc1ea5a17b39d7854bcd0c06e3" STREQUAL "")
  message(FATAL_ERROR "EXPECT_VALUE can't be empty")
endif()

message(VERBOSE "verifying file...
     file='/home/karry/SailfishOS/projects/harbour-camera-buddy/dependencies/libtool-2.4.6.tar.gz'")

file("SHA256" "/home/karry/SailfishOS/projects/harbour-camera-buddy/dependencies/libtool-2.4.6.tar.gz" actual_value)

if(NOT "${actual_value}" STREQUAL "e3bd4d5d3d025a36c21dd6af7ea818a2afcd4dfc1ea5a17b39d7854bcd0c06e3")
  message(FATAL_ERROR "error: SHA256 hash of
  /home/karry/SailfishOS/projects/harbour-camera-buddy/dependencies/libtool-2.4.6.tar.gz
does not match expected value
  expected: 'e3bd4d5d3d025a36c21dd6af7ea818a2afcd4dfc1ea5a17b39d7854bcd0c06e3'
    actual: '${actual_value}'
")
endif()

message(VERBOSE "verifying file... done")
