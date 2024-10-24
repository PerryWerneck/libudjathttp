#
# spec file for package libudjathttp
#
# Copyright (c) 2015 SUSE LINUX GmbH, Nuernberg, Germany.
# Copyright (C) <2008> <Banco do Brasil S.A.>
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via http://bugs.opensuse.org/
#

%define product_name %(pkg-config --variable=product_name libudjat)
%define module_path %(pkg-config --variable=module_path libudjat)

Summary:		%{product_name} http client library 
Name:			udjat-module-http
Version:		1.2.0+git20241024
Release:		0
License:		LGPL-3.0
Source:			%{name}-%{version}.tar.xz

%define MAJOR_VERSION %(echo %{version} | cut -d. -f1)
%define MINOR_VERSION %(echo %{version} | cut -d. -f2 | cut -d+ -f1)
%define _libvrs %{MAJOR_VERSION}_%{MINOR_VERSION}

URL:			https://github.com/PerryWerneck/udjat-module-http.git

Group:			Development/Libraries/C and C++
BuildRoot:		/var/tmp/%{name}-%{version}

BuildRequires:	autoconf >= 2.61
BuildRequires:	automake
BuildRequires:	libtool
BuildRequires:	binutils
BuildRequires:	coreutils
BuildRequires:	gcc-c++

BuildRequires:	pkgconfig(libudjat)
BuildRequires:	pkgconfig(libcurl)

%define MAJOR_VERSION %(echo %{version} | cut -d. -f1)
%define MINOR_VERSION %(echo %{version} | cut -d. -f2 | cut -d+ -f1)

Provides:		udjat-protocol-http

%description
HTTP client module and library for %{product_name}

%package -n libudjathttp%{_libvrs}
Summary: %{product_name} http client library

%description -n libudjathttp%{_libvrs}
HTTP client libbrary for %{product_name}

%package -n libudjathttp-devel
Summary:        Development files for %{name}
Requires:       pkgconfig(libudjat)
Requires:       libudjathttp%{_libvrs} = %{version}

%description -n libudjathttp-devel

Development files for %{product_name}'s HTTP client library.

#---[ Build & Install ]-----------------------------------------------------------------------------------------------

%prep
%setup

NOCONFIGURE=1 \
	./autogen.sh

%configure 

%build
make all

%install
%makeinstall

%files
%{module_path}/*.so

%files -n libudjathttp%{_libvrs}
%defattr(-,root,root)
%{_libdir}/lib*.so.%{MAJOR_VERSION}.%{MINOR_VERSION}

%files -n libudjathttp-devel
%defattr(-,root,root)
%{_includedir}/udjat/tools/http/*.h
%{_libdir}/*.so
%{_libdir}/*.a
%{_libdir}/pkgconfig/*.pc

%changelog

