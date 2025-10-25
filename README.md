# Camera Buddy for SailfishOS

[![Last build on Ubuntu 24.04](https://github.com/Karry/harbour-camera-buddy/actions/workflows/build.yml/badge.svg)](https://github.com/Karry/harbour-camera-buddy/actions/workflows/build.yml)

Camera Buddy is a SailfishOS application that provides a user-friendly interface for connecting to external DSLR cameras
via USB or WiFi using the GPhoto2 library. It allows you to browse and download photos from connected cameras directly to your SailfishOS device.

## Disclaimer

This application was vibe-coded using GitHub Copilot AI assistant based on provided instructions and my manual corrections.
It was my experiment howto recent AI tools can handle C++/QML development for minor platform like SailfishOS.
Code is ugly and ~probably~ DEFINITELY contains bugs, but at least it compiles and basic functionality works.
It may explode! You have been warned.

<img alt="List of connected cameras"
    width="270" height="630"
    src="https://raw.githubusercontent.com/Karry/harbour-camera-buddy/master/graphics/cameras.png" />
<img alt="Photo browsing"
    width="270" height="630"
    src="https://raw.githubusercontent.com/Karry/harbour-camera-buddy/master/graphics/photos.png" />
<img alt="Downloading"
    width="270" height="630"
    src="https://raw.githubusercontent.com/Karry/harbour-camera-buddy/master/graphics/downloading.png" />

## Features

- **Camera Detection**: Automatically detect connected DSLR cameras
- **Photo Browsing**: View photos stored on your camera
- **Batch Download**: Select and download multiple photos at once
- **Progress Tracking**: Monitor download progress in real-time
- **SailfishOS Native**: Built specifically for SailfishOS with native UI components

## Build Status

The project includes automated builds and testing via GitHub Actions:
- **Ubuntu 24.04 Build**: Verifies compilation with latest dependencies

## Dependencies

- GPhoto2 library (for camera communication)
- libusb (USB camera support)
- Qt5 (Core, DBus, Multimedia, Qml, Quick, Svg)
- SailfishApp framework
- Silica UI components

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

