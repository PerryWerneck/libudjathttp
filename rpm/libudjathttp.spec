#
# spec file for package libudjathttp
#
# Copyright (c) <2024> Perry Werneck <perry.werneck@gmail.com>.
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via https://github.com/PerryWerneck/libudjathttp/issues
#

%define module_name http

%define product_name %(pkg-config --variable=product_name libudjat)
%define product_version %(pkg-config --variable=product_version libudjat)
%define module_path %(pkg-config --variable=module_path libudjat)

Summary:		HTTP client library for %{product_name}  
Name:			libudjat%{module_name}
Version: 2.0.0
Release:		0
License:		LGPL-3.0
Source:			%{name}-%{version}.tar.xz

URL:			https://github.com/PerryWerneck/libudjat%{module_name}

Group:			Development/Libraries/C and C++
BuildRoot:		/var/tmp/%{name}-%{version}

BuildRequires:	binutils
BuildRequires:	coreutils

%if "%{_vendor}" == "debbuild"
BuildRequires:  meson-deb-macros
BuildRequires:	curl-dev
BuildRequires:	libudjat-dev
%else
BuildRequires:	gcc-c++ >= 5
BuildRequires:	pkgconfig(libcurl)
BuildRequires:	pkgconfig(libudjat)
%endif

%if 0%{?suse_version} == 01500
BuildRequires:  meson = 0.61.4
%else
BuildRequires:  meson
%endif

%description
HTTP client library for %{product_name}

C++ HTTP client classes for use with lib%{product_name}

#---[ Library ]-------------------------------------------------------------------------------------------------------

%define MAJOR_VERSION %(echo %{version} | cut -d. -f1)
%define MINOR_VERSION %(echo %{version} | cut -d. -f2 | cut -d+ -f1)
%define _libvrs %{MAJOR_VERSION}_%{MINOR_VERSION}

%package -n %{name}%{_libvrs}
Summary: HTTP client library for %{product_name}

%description -n %{name}%{_libvrs}
HTTP client library for %{product_name}

C++ HTTP client classes for use with lib%{product_name}

#---[ Development ]---------------------------------------------------------------------------------------------------

%package devel
Summary: Development files for %{name}
Requires: %{name}%{_libvrs} = %{version}

%if "%{_vendor}" == "debbuild"
Provides:	%{name}-dev
Provides:	pkgconfig(%{name})
Provides:	pkgconfig(%{name}-static)
%endif

%description devel
HTTP client library for %{product_name}

C++ HTTP client classes for use with lib%{product_name}

%lang_package -n %{name}%{_libvrs}

#---[ Module ]--------------------------------------------------------------------------------------------------------

%package -n %{product_name}-module-%{module_name}
Summary: HTTP module for %{name}

%description -n %{product_name}-module-%{module_name}
%{product_name} module with http client support.

#---[ Build & Install ]-----------------------------------------------------------------------------------------------

%prep
%autosetup
%meson

%build
%meson_build

%install
%meson_install
%find_lang %{name}-%{MAJOR_VERSION}.%{MINOR_VERSION} langfiles

%files -n %{name}%{_libvrs}
%defattr(-,root,root)
%{_libdir}/%{name}.so.%{MAJOR_VERSION}.%{MINOR_VERSION}

%files -n %{name}%{_libvrs}-lang -f langfiles

%files -n %{product_name}-module-%{module_name}
%{module_path}/*.so

%files devel
%defattr(-,root,root)

%{_libdir}/*.so
%{_libdir}/*.a
%{_libdir}/pkgconfig/*.pc

%dir %{_includedir}/udjat/tools/http
%{_includedir}/udjat/tools/http/*.h

%post -n %{name}%{_libvrs} -p /sbin/ldconfig

%postun -n %{name}%{_libvrs} -p /sbin/ldconfig

%changelog

