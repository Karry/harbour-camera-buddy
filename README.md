# harbour-camera-buddy

![Build Status](https://github.com/YOUR_USERNAME/harbour-camera-buddy/workflows/Build%20Camera%20Buddy/badge.svg)

Sailfish OS UI for Gphoto2

## Overview

Camera Buddy is a SailfishOS application that provides a user-friendly interface for connecting to external DSLR cameras via USB or WiFi using the GPhoto2 library. It allows you to browse and download photos from connected cameras directly to your SailfishOS device.

## Features

- **Camera Detection**: Automatically detect connected DSLR cameras
- **Photo Browsing**: View photos stored on your camera
- **Batch Download**: Select and download multiple photos at once
- **Progress Tracking**: Monitor download progress in real-time
- **SailfishOS Native**: Built specifically for SailfishOS with native UI components

## Build Status

The project includes automated builds and testing via GitHub Actions:
- **Ubuntu 24.04 Build**: Verifies compilation with latest dependencies
- **Code Quality**: Static analysis and formatting checks
- **Structure Validation**: Ensures SailfishOS packaging requirements

## Dependencies

- GPhoto2 library (for camera communication)
- libusb (USB camera support)
- Qt5 (Core, DBus, Multimedia, Qml, Quick, Svg)
- SailfishApp framework

## Building

### Local Development Build
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### SailfishOS SDK Build
```bash
./deploy.sh
```

This will build the project in the official SailfishOS SDK environment and prepare it for deployment.

