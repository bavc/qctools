%define qctools_version           1.3
%define debug_package %{nil}
%define _unpackaged_files_terminate_build 0

Name:           qctools
Version:        %{qctools_version}
Release:        1
Summary:        QCTools

Group:          Applications/Multimedia
License:        BSD-2-Clause
URL:            http://MediaArea.net
Packager:       MediaArea.net SARL <info@mediaarea.net>
Source0:        %{name}_%{version}-1.tar.gz

Prefix:         %{_prefix}
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root
BuildRequires:  gcc-c++
BuildRequires:  pkgconfig
BuildRequires:  zlib-devel
%if 0%{?fedora_version} >= 24
BuildRequires: bzip2-devel
%else
%if ! 0%{?mageia}
BuildRequires:  libbz2-devel
%endif
%endif
BuildRequires:  yasm
BuildRequires:  cmake
%if 0%{?suse_version}
BuildRequires:  update-desktop-files
%endif

%if 0%{?rhel} > 7
BuildRequires:  alternatives
%endif

%if 0%{?suse_version} >= 1200
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Svg)
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Widgets)
BuildRequires:  pkgconfig(Qt5Network)
BuildRequires:  pkgconfig(Qt5Concurrent)
BuildRequires:  pkgconfig(Qt5PrintSupport)
BuildRequires:  pkgconfig(Qt5QuickControls2)
BuildRequires:  pkgconfig(Qt5XmlPatterns)
BuildRequires:  pkgconfig(Qt5Multimedia)
BuildRequires:  libXv-devel
%endif

%if 0%{?fedora_version} ||  0%{?rhel} > 7
BuildRequires:  pkgconfig(Qt5)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5QuickControls2)
BuildRequires:  pkgconfig(Qt5Svg)
BuildRequires:  pkgconfig(Qt5XmlPatterns)
BuildRequires:  pkgconfig(Qt5Multimedia)
BuildRequires:  desktop-file-utils
BuildRequires:  libXv-devel
%endif

%if 0%{?mageia}
%ifarch x86_64
BuildRequires:  lib64bzip2-devel
BuildRequires:  lib64qt5qml-devel
BuildRequires:  lib64qt5base5-devel
BuildRequires:  lib64qt5quicktemplates2-devel
BuildRequires:  lib64qt5quicktemplates2_5
BuildRequires:  lib64qt5quickcontrols2-devel
BuildRequires:  lib64qt5quickcontrols2_5
BuildRequires:  lib64qt5quickwidgets-devel
BuildRequires:  lib64qt5multimedia-devel
BuildRequires:  lib64qt5svg-devel
BuildRequires:  lib64qt5xmlpatterns-devel
BuildRequires:  lib64qt5xmlpatterns5
%else
BuildRequires:  libbzip2-devel
BuildRequires:  libqt5qml-devel
BuildRequires:  libqt5base5-devel
BuildRequires:  libqt5quicktemplates2-devel
BuildRequires:  libqt5quicktemplates2_5
BuildRequires:  libqt5quickcontrols2-devel
BuildRequires:  libqt5quickcontrols2_5
BuildRequires:  libqt5quickwidgets-devel
BuildRequires:  libqt5multimedia-devel
BuildRequires:  libqt5svg-devel
BuildRequires:  libqt5xmlpatterns-devel
BuildRequires:  libqt5xmlpatterns5
%endif
%if 0%{?mageia} > 5
BuildRequires:  libproxy-pacrunner
%endif
BuildRequires:  sane-backends-iscan
BuildRequires:  libuuid-devel
%endif

%package -n qcli
Summary:  QCTools command line interface
Group:    Applications/Multimedia

%description
QCTools (Quality Control Tools for Video Preservation)

QCTools (Quality Control Tools for Video Preservation) is a free and open source software tool
that helps users analyze and understand their digitized video files through use of audiovisual analytics and filtering
to help users detect corruptions or compromises in the results of analog video digitization or in born-digital video.
The goal of the project is to cut down the time it takes to perform high-quality video preservation
and direct time towards preservation issues that are solvable - for example, identifying tapes
that would benefit from a second transfer, saving not only the precious time of preservationists
and institutional resources, but giving collections a necessary advantage in the bigger race against time
to preserve their significant cultural artifacts. QCTools incorporates archival standards and best practices
for reformatting and capturing metadata that enables the long-term preservation of and access to the original artifact,
the digital object, and the associated catalog record.

%description -n qcli
QCli - QCTools Command Line Interface

QCTools (Quality Control Tools for Video Preservation) is a free and open source software tool
that helps users analyze and understand their digitized video files through use of audiovisual analytics and filtering
to help users detect corruptions or compromises in the results of analog video digitization or in born-digital video.
The goal of the project is to cut down the time it takes to perform high-quality video preservation
and direct time towards preservation issues that are solvable - for example, identifying tapes
that would benefit from a second transfer, saving not only the precious time of preservationists
and institutional resources, but giving collections a necessary advantage in the bigger race against time
to preserve their significant cultural artifacts. QCTools incorporates archival standards and best practices
for reformatting and capturing metadata that enables the long-term preservation of and access to the original artifact,
the digital object, and the associated catalog record.

%prep
%setup -q -n qctools

# build
pushd qctools
    chmod 644 History.txt
    chmod 644 License.html
    ./Project/BuildAllFromSource/build
popd

%install
install -dm 755 %{buildroot}%{_bindir}
install -m 755 qctools/Project/QtCreator/build/qctools-gui/QCTools %{buildroot}%{_bindir}
install -m 755 qctools/Project/QtCreator/build/qctools-cli/qcli %{buildroot}%{_bindir}

# icon
install -dm 755 %{buildroot}%{_datadir}/icons/hicolor/256x256/apps
install -m 644 qctools/Source/Resource/Logo.png %{buildroot}%{_datadir}/icons/hicolor/256x256/apps/%{name}.png
install -dm 755 %{buildroot}%{_datadir}/pixmaps
install -m 644 qctools/Source/Resource/Logo.png %{buildroot}%{_datadir}/pixmaps/%{name}.png

# menu-entry
install -dm 755 %{buildroot}/%{_datadir}/applications
install -m 644 qctools/Project/GNU/GUI/qctools.desktop %{buildroot}/%{_datadir}/applications
%if 0%{?suse_version}
  %suse_update_desktop_file -n qctools AudioVideo AudioVideoEditing
%endif
install -dm 755 %{buildroot}/%{_datadir}/apps/konqueror/servicemenus
install -m 644 qctools/Project/GNU/GUI/qctools.kde3.desktop %{buildroot}/%{_datadir}/apps/konqueror/servicemenus/qctools.desktop
%if 0%{?suse_version}
  %suse_update_desktop_file -n %{buildroot}/%{_datadir}/apps/konqueror/servicemenus/qctools.desktop AudioVideo AudioVideoEditing
%endif
install -dm 755 %{buildroot}/%{_datadir}/kde4/services/ServiceMenus/
install -m 644 qctools/Project/GNU/GUI/qctools.kde4.desktop \
    %{buildroot}/%{_datadir}/kde4/services/ServiceMenus/qctools.desktop
install -dm 755 %{buildroot}/%{_datadir}/kservices5/ServiceMenus/
install -m 644 qctools/Project/GNU/GUI/qctools.kde4.desktop \
    %{buildroot}/%{_datadir}/kservices5/ServiceMenus/qctools.desktop
%if 0%{?suse_version}
  %suse_update_desktop_file -n %{buildroot}/%{_datadir}/kde4/services/ServiceMenus/qctools.desktop AudioVideo AudioVideoEditing
  %suse_update_desktop_file -n %{buildroot}/%{_datadir}/kservices5/ServiceMenus/qctools.desktop AudioVideo AudioVideoEditing
%endif
%if %{undefined fedora_version} || 0%{?fedora_version} < 26
install -dm 755 %{buildroot}%{_datadir}/appdata/
install -m 644 qctools/Project/GNU/GUI/qctools.metainfo.xml %{buildroot}%{_datadir}/appdata/qctools.appdata.xml
%else
install -dm 755 %{buildroot}%{_datadir}/metainfo/
install -m 644  qctools/Project/GNU/GUI/qctools.metainfo.xml %{buildroot}%{_datadir}/metainfo/qctools.metainfo.xml
%endif

%files
%defattr(-,root,root,-)
%doc qctools/License.html qctools/History.txt
%{_bindir}/QCTools
%{_datadir}/applications/*.desktop
%{_datadir}/pixmaps/*.png
%dir %{_datadir}/icons/hicolor
%dir %{_datadir}/icons/hicolor/256x256
%dir %{_datadir}/icons/hicolor/256x256/apps
%{_datadir}/icons/hicolor/256x256/apps/*.png
%dir %{_datadir}/apps
%dir %{_datadir}/apps/konqueror
%dir %{_datadir}/apps/konqueror/servicemenus
%{_datadir}/apps/konqueror/servicemenus/*.desktop
%dir %{_datadir}/kde4
%dir %{_datadir}/kde4/services
%dir %{_datadir}/kde4/services/ServiceMenus
%{_datadir}/kde4/services/ServiceMenus/*.desktop
%dir %{_datadir}/kservices5
%dir %{_datadir}/kservices5/ServiceMenus
%{_datadir}/kservices5/ServiceMenus/*.desktop
%if 0%{?fedora_version} && 0%{?fedora_version} >= 26
%dir %{_datadir}/metainfo
%{_datadir}/metainfo/*.xml
%else
%dir %{_datadir}/appdata
%{_datadir}/appdata/*.xml
%endif

%files -n qcli
%defattr(-,root,root,-)
%doc qctools/License.html qctools/History.txt
%{_bindir}/qcli

%changelog
* Wed Jan 01 2014 MediaArea.net SARL <info@mediaarea.net> - 0.5.0
- See History.txt for more info and real dates
