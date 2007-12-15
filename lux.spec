Name:           lux
Version:        0.1
Release:        RC4
Summary:        Lux Renderer

Group:          Applications/Multimedia
License:        GPL
URL:            http://www.luxrender2.org
Source0:        %{name}-%{version}.tar.bz2
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  libpng-devel
Requires:       libpng

%description
LuxRender is a  rendering system for physically correct, unbiased image synthesis.
Rendering with LuxRender means simulating the flow of light according to physical equations.
This produces realistic, photographic-quality images. 

%prep
%setup -q


%build
cmake .
make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT


%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,-)
%doc AUTHORS.txt COPYING.txt
%{_bindir}/luxconsole
%{_bindir}/luxrender



%changelog