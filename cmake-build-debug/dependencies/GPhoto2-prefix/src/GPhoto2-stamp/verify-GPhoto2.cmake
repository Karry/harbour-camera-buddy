# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

if("/home/karry/SailfishOS/projects/harbour-camera-buddy/dependencies/libgphoto2-2.5.29.tar.gz" STREQUAL "")
  message(FATAL_ERROR "LOCAL can't be empty")
endif()

if(NOT EXISTS "/home/karry/SailfishOS/projects/harbour-camera-buddy/dependencies/libgphoto2-2.5.29.tar.gz")
  message(FATAL_ERROR "File not found: /home/karry/SailfishOS/projects/harbour-camera-buddy/dependencies/libgphoto2-2.5.29.tar.gz")
endif()

if("SHA256" STREQUAL "")
  message(WARNING "File cannot be verified since no URL_HASH specified")
  return()
endif()

if("467eaf95e5385e48786ee115802e24ece00f34ae88ac02f5dfa7c836846c6294" STREQUAL "")
  message(FATAL_ERROR "EXPECT_VALUE can't be empty")
endif()

message(VERBOSE "verifying file...
     file='/home/karry/SailfishOS/projects/harbour-camera-buddy/dependencies/libgphoto2-2.5.29.tar.gz'")

file("SHA256" "/home/karry/SailfishOS/projects/harbour-camera-buddy/dependencies/libgphoto2-2.5.29.tar.gz" actual_value)

if(NOT "${actual_value}" STREQUAL "467eaf95e5385e48786ee115802e24ece00f34ae88ac02f5dfa7c836846c6294")
  message(FATAL_ERROR "error: SHA256 hash of
  /home/karry/SailfishOS/projects/harbour-camera-buddy/dependencies/libgphoto2-2.5.29.tar.gz
does not match expected value
  expected: '467eaf95e5385e48786ee115802e24ece00f34ae88ac02f5dfa7c836846c6294'
    actual: '${actual_value}'
")
endif()

message(VERBOSE "verifying file... done")
