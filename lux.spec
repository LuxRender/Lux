Name:           lux
Version:        0.1
Release:        rc4%{?dist}
Summary:        Lux Renderer, an unbiased rendering system

Group:          Applications/Multimedia
License:        GPLv3
URL:            http://www.luxrender2.org
Packager:	J.F. Romang <jeanfrancois.romang@laposte.net>
Source0:        %{name}-%{version}.tar.bz2
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  libpng-devel libjpeg-devel libtiff-devel fltk-devel OpenEXR-devel flex bison boost-devel desktop-file-utils cmake
Requires:       libpng libjpeg libtiff fltk OpenEXR-libs OpenEXR ilmbase boost

%description
LuxRender is a rendering system for physically correct image synthesis. 

%prep
%setup -q


%build
cmake .
make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
desktop-file-install --vendor="" --dir=%{buildroot}%{_datadir}/applications/ renderer/lux.desktop

%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,-)
%doc AUTHORS.txt COPYING.txt
%{_bindir}/luxconsole
%{_bindir}/luxrender
%{_datadir}/pixmaps/lux.png
%{_datadir}/applications/lux.desktop


%changelog
*Mon Dec 17 2007 Romang Jean-Francois <jeanfrancois.romang@laposte.net> 0.1-rc4
-Initial version