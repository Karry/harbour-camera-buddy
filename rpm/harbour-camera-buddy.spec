Name:       harbour-camera-buddy

# >> macros

# ignore installed files that are not packed to rpm
%define _unpackaged_files_terminate_build 0

# don't setup rpm provides
%define __provides_exclude_from ^%{_datadir}/.*$

# don't setup rpm requires
# list here all the libraries your RPM installs
%define __requires_exclude ^ld-linux|libMagick*|libgphoto2*|libtimelapse.so|libv4l*|libvidstab.so.*|libgd.so.*|libjpeg.so.*|libltdl.so.*|libusb-1.0.so.*$

# << macros

Summary:    Camera Buddy for Sailfish OS
Version:    0.1.0
Release:    1
Group:      Qt/Qt
License:    GPL-2.0-or-later
Source0:    %{name}-%{version}.tar.bz2
URL:        https://github.com/karry/harbour-camera-buddy
BuildRequires:  pkgconfig(sailfishapp) >= 1.0.2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(Qt5Multimedia)
BuildRequires:  pkgconfig(Qt5Svg)
BuildRequires:  qt5-qttools-linguist
BuildRequires:  desktop-file-utils
BuildRequires:  cmake >= 3.9.2
BuildRequires:  gcc-c++
BuildRequires:  pkgconfig
BuildRequires:  autoconf
BuildRequires:  automake
BuildRequires:  libtool
BuildRequires:  gettext-devel

%description
Camera Buddy is a Sailfish OS application for controlling external cameras via USB.
It provides remote camera control, image transfer, and camera settings management
using the GPhoto2 library. The application allows photographers to remotely control
their cameras, transfer images, and manage camera settings directly from their
Sailfish OS device.

Features:
- Camera detection and connection via USB
- Remote camera control (shutter, settings)
- Image preview and transfer
- Camera settings management
- Support for multiple camera models via GPhoto2

%prep
%setup -q -n %{name}-%{version}

%build
# >> build pre
#rm -rf rpmbuilddir-%{_arch}

## for production build:
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DQT_QML_DEBUG=no -DSANITIZER=none -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_INSTALL_RPATH=%{_datadir}/%{name}/lib/: -S . -B rpmbuilddir-%{_arch}
## for debug build, use these cmake arguments instead:
# cmake -DCMAKE_BUILD_TYPE=DEBUG -DQT_QML_DEBUG=yes -DSANITIZER=none -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_INSTALL_RPATH=%{_datadir}/%{name}/lib/: ..

make -C rpmbuilddir-%{_arch} VERBOSE=1 %{?_smp_mflags}
# << build pre

# >> build post
# << build post

%install
# >> install pre
#rm -rf %{buildroot}
DESTDIR=%{buildroot} make -C rpmbuilddir-%{_arch} install
mkdir -p %{_bindir}
# << install pre

# >> install post

desktop-file-install --delete-original       \
  --dir %{buildroot}%{_datadir}/applications             \
   %{buildroot}%{_datadir}/applications/*.desktop

## Jolla harbour rules:

# -- ship all shared unallowed libraries with the app
mkdir -p %{buildroot}%{_datadir}/%{name}/lib

# << install post

# -- check app rpath to find its libs
chrpath -l %{buildroot}%{_bindir}/%{name}
ls -al     %{buildroot}%{_bindir}/%{name}
sha1sum    %{buildroot}%{_bindir}/%{name}

%files
%defattr(-,root,root,-)
%{_bindir}
%{_datadir}/%{name}
%{_datadir}/%{name}/lib/
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/86x86/apps/%{name}.png
%{_datadir}/icons/hicolor/108x108/apps/%{name}.png
%{_datadir}/icons/hicolor/128x128/apps/%{name}.png
%{_datadir}/icons/hicolor/172x172/apps/%{name}.png
%{_datadir}/icons/hicolor/256x256/apps/%{name}.png
# >> files
# << files

%changelog
* Wed Oct 22 2025 Lukas Karas <lukas.karas@centrum.cz> - 0.1.0-1
- Initial release
- USB and WiFi (PTP/IP) camera support, using GPhoto library, see https://gphoto.sourceforge.io/proj/libgphoto2/support.php
- Detect connected cameras, image transfer
