
Name: libosmscout-qt

Summary: libosmscout qt libraries
Version: 0.0.git.20160915
Release: 1
Group: Qt/Qt
License: LGPL
Source0:    %{name}-%{version}.tar.bz2

Requires: protobuf

BuildRequires: cmake
BuildRequires: pkgconfig(Qt5Core)
BuildRequires: protobuf-devel

%description
libosmscout qt libraries

%build
%cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr -DOSMSCOUT_BUILD_MAP_OPENGL=OFF -DOSMSCOUT_BUILD_IMPORT=ON -DOSMSCOUT_BUILD_MAP_AGG=OFF -DOSMSCOUT_BUILD_MAP_CAIRO=OFF -DOSMSCOUT_BUILD_MAP_SVG=OFF -DOSMSCOUT_BUILD_MAP_IOSX=OFF -DOSMSCOUT_BUILD_TESTS=OFF -DOSMSCOUT_BUILD_DEMOS=OFF -DOSMSCOUT_BUILD_BINDING_JAVA=OFF -DOSMSCOUT_BUILD_BINDING_CSHARP=OFF -DOSMSCOUT_BUILD_DOC_API=OFF -DOSMSCOUT_BUILD_CLIENT_QT=OFF -DOSMSCOUT_BUILD_TOOL_OSMSCOUT2=OFF -DOSMSCOUT_BUILD_TOOL_STYLEEDITOR=OFF -DGPERFTOOLS_USAGE=OFF .
make %{?_smp_mflags}

%install
make install DESTDIR=%{buildroot}

%check
ctest -V %{?_smp_mflags}
