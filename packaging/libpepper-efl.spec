Name:       libpepper-efl
Version:    0.0.11
Release:    0
Summary:    EFL backend for pepper
License:    MIT
Group:      Graphics & UI Framework/Wayland Window System

Source:     %{name}-%{version}.tar.xz
Source1001: %{name}.manifest

BuildRequires: pkgconfig(wayland-server)
BuildRequires: pkgconfig(pepper)
BuildRequires: pkgconfig(elementary)
BuildRequires: pkgconfig(xdg-shell-server)
BuildRequires: pkgconfig(xkbcommon)
BuildRequires: pkgconfig(tizen-extension-client)
BuildRequires: pkgconfig(ecore-wayland)
BuildRequires: pkgconfig(wayland-tbm-server)
BuildRequires: pkgconfig(wayland-tbm-client)
Requires: libwayland-extension-server

%description
EFL backend for pepper.
Pepper is a lightweight and flexible library for developing various types of wayland compositors.

%prep
%setup -q
cp %{SOURCE1001} .

%build
%autogen --enable-examples
make %{?_smp_mflags}

%install
%make_install

%files -n %{name}
%manifest %{name}.manifest
%defattr(-,root,root,-)
%dir %{_includedir}/pepper-efl/
%{_includedir}/pepper-efl/*.h
%{_libdir}/libpepper-efl.so.*
%{_libdir}/libpepper-efl.so
%{_libdir}/pkgconfig/libpepper-efl.pc
%{_bindir}/*
%{_libexecdir}/*
%{_datadir}/%{name}
