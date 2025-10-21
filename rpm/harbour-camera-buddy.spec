Name:       harbour-camera-buddy

%{!?qtc_qmake:%define qtc_qmake %qmake}
%{!?qtc_qmake5:%define qtc_qmake5 %qmake5}
%{!?qtc_make:%define qtc_make make}
%{?qtc_builddir:%define _builddir %qtc_builddir}

Summary:    Camera Buddy for Sailfish OS
Version:    0.1.0
Release:    1
Group:      Qt/Qt
License:    GPL-2.0-or-later
Source0:    %{name}-%{version}.tar.bz2
URL:        https://github.com/karry/harbour-camera-buddy
Requires:   sailfishsilica-qt5 >= 0.10.9
Requires:   qt5-qtmultimedia
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
%cmake -DCMAKE_BUILD_TYPE=Release
%make_build

%install
%make_install
desktop-file-install --delete-original \
  --dir %{buildroot}%{_datadir}/applications \
   %{buildroot}%{_datadir}/applications/*.desktop

%files
%defattr(-,root,root,-)
%license LICENSE
%{_bindir}/%{name}
%{_datadir}/%{name}/
%{_datadir}/applications/%{name}.desktop

%changelog
* Mon Oct 21 2024 Camera Buddy Team <team@camera-buddy.org> - 0.1.0-1
- Initial release
- GPhoto2 camera control support
- Basic UI for camera detection and control
- USB camera connection support
