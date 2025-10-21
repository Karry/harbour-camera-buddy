#!/bin/bash -xe
cd "$(dirname "$0")"
if [ ! -d "harbour-timelapse-tools" ]; then
  git clone --recursive https://github.com/Karry/harbour-timelapse-tools.git
fi
