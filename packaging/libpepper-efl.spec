Name:       libpepper-efl
Version:    0.0.1
Release:    0
Summary:    EFL backend for pepper
License:    MIT
Group:      Graphics & UI Framework/Wayland Window System

Source:     %{name}-%{version}.tar.xz

BuildRequires: pkgconfig(wayland-server)
BuildRequires: pkgconfig(pepper)
BuildRequires: pkgconfig(elementary)
BuildRequires: pkgconfig(xdg-shell-server)
BuildRequires: pkgconfig(xkbcommon)
BuildRequires: pkgconfig(tizen-extension-client)
BuildRequires: pkgconfig(ecore-wayland)
Requires: libwayland-extension-server

%description
EFL backend for pepper.
Pepper is a lightweight and flexible library for developing various types of wayland compositors.

%prep
%setup -q
                      
%build
%autogen --enable-examples
make %{?_smp_mflags}

%install
%make_install

%files -n %{name}        
%defattr(-,root,root,-)
%dir %{_includedir}/pepper-efl/                                                      
%{_includedir}/pepper-efl/*.h 
%{_libdir}/libpepper-efl.so.*
%{_libdir}/libpepper-efl.so
%{_libdir}/pkgconfig/libpepper-efl.pc
%{_bindir}/*
%{_libexecdir}/*
%{_datadir}/%{name}
