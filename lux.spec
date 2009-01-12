Name:           lux
Version:        0.6
Release:        1
Summary:        Lux Renderer, an unbiased rendering system

Group:          Applications/Multimedia
License:        GPLv3
URL:            http://www.luxrender.net
Packager:		J.F. Romang <jeanfrancois.romang@laposte.net>
Source0:        %{name}-%{version}.tar.bz2
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

%if 0%{?suse_version}
BuildRequires:  libpng-devel libjpeg-devel libtiff-devel OpenEXR-devel flex bison boost-devel desktop-file-utils wxGTK-devel gcc gcc-c++ Mesa-devel cmake update-desktop-files
Requires:       libpng libjpeg libtiff OpenEXR IlmBase boost-devel wxGTK
%else

%if 0%{?mandriva_version} 
BuildRequires:  libpng-devel libjpeg-devel libtiff-devel OpenEXR-devel flex bison boost-devel desktop-file-utils libwxgtk2.8-devel gcc gcc-c++ mesa-common-devel cmake
Requires:       libpng libjpeg libtiff OpenEXR IlmBase boost libwxgtk2.8
%else
BuildRequires:  libpng-devel libjpeg-devel libtiff-devel OpenEXR-devel flex bison boost-devel desktop-file-utils wxGTK-devel gcc gcc-c++ Mesa-devel cmake
Requires:       libpng libjpeg libtiff OpenEXR IlmBase boost wxGTK
%endif

%endif



%description
LuxRender is a rendering system for physically correct image synthesis. 

%prep
%setup -q


%build
cmake . -DCMAKE_INSTALL_PREFIX=/usr
make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
desktop-file-install --vendor="" --dir=%{buildroot}%{_datadir}/applications/ renderer/luxrender.desktop
%if 0%{?suse_version}
%suse_update_desktop_file luxrender
%endif

%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,-)
%doc AUTHORS.txt COPYING.txt
%dir %{_includedir}/luxrender
%{_bindir}/luxconsole
%{_bindir}/luxrender
%{_datadir}/pixmaps/luxrender.svg
%{_datadir}/applications/luxrender.desktop
%{_libdir}/liblux.a
%{_includedir}/luxrender/api.h


%changelog
*Sat Jan 10 2009 Romang Jean-Francois <jeanfrancois.romang@laposte.net> 0.6-beta1
-Changes to use wxWidgets GUI
-Solved /usr/lib64 path problem
*Mon Dec 17 2007 Romang Jean-Francois <jeanfrancois.romang@laposte.net> 0.1-rc4
-Initial version
