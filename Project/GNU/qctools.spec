%define qctools_version           1.4

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
BuildRequires:  nasm
BuildRequires:  gcc-c++
BuildRequires:  pkgconfig
BuildRequires:  zlib-devel
BuildRequires:  freetype-devel
BuildRequires:  harfbuzz-devel

%if 0%{?fedora_version} ||  0%{?rhel}
BuildRequires:  pkgconfig(Qt6)
BuildRequires:  pkgconfig(Qt6Svg)
BuildRequires:  pkgconfig(Qt6Multimedia)
BuildRequires:  pkgconfig(Qt6Qml)
BuildRequires:  pkgconfig(Qt6QuickControls2)
BuildRequires:  desktop-file-utils
%endif

%if 0%{?mageia}
BuildRequires:  lib64bzip2-devel
BuildRequires:  lib64qt6base6-devel
BuildRequires:  lib64qt6svg-devel
BuildRequires:  lib64qt6qml-devel
BuildRequires:  lib64qt6multimedia-devel
BuildRequires:  lib64qt6multimediawidgets-devel
BuildRequires:  lib64qt6quicktemplates2-devel
BuildRequires:  lib64qt6quicktemplates26
BuildRequires:  lib64qt6quickcontrols2-devel
BuildRequires:  lib64qt6quickcontrols26
BuildRequires:  lib64qt6quickwidgets-devel
%endif

%if 0%{?suse_version}
BuildRequires:  pkgconfig(Qt6Qml)
BuildRequires:  pkgconfig(Qt6Svg)
BuildRequires:  pkgconfig(Qt6Core)
BuildRequires:  pkgconfig(Qt6Widgets)
BuildRequires:  pkgconfig(Qt6Network)
BuildRequires:  pkgconfig(Qt6Concurrent)
BuildRequires:  pkgconfig(Qt6PrintSupport)
BuildRequires:  pkgconfig(Qt6QuickControls2)
BuildRequires:  pkgconfig(Qt6Multimedia)
BuildRequires:  pkgconfig(Qt6MultimediaWidgets)
BuildRequires:  update-desktop-files
%endif

%if 0%{?fedora_version} > 39
BuildRequires:  libvpl
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
pushd ffmpeg
    ./configure --prefix="$(pwd)../output" --enable-gpl --enable-version3 --disable-autodetect --disable-programs --enable-static --disable-shared --disable-doc --disable-debug --enable-libfreetype --enable-libharfbuzz
    %__make %{?jobs:-j%{jobs}}
popd

pushd qwt
    patch -p1 < ../qctools/Project/BuildAllFromSource/qwt.patch
    export QWT_STATIC=1 QWT_NO_SVG=1 QWT_NO_OPENGL=1 QWT_NO_DESIGNER=1
    qmake6
    %__make %{?jobs:-j%{jobs}}
popd

pushd qctools
    chmod 644 History.txt
    chmod 644 License.html
    mkdir Project/QtCreator/build
    pushd Project/QtCreator/build
    qmake6 .. -after CONFIG+=force_debug_info LIBS+=-lharfbuzz LIBS+=-lfreetype
    %__make %{?jobs:-j%{jobs}}
    popd
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
* Thu Mar 13 2025 MediaArea.net SARL <info@mediaarea.net> - 25.03
- See History.txt for more information
