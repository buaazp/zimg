Name:     zimg
Version:  3.1.0
Release:  Release
Summary:  zimg - A light and high performance image storage and processing system.
Group:    System Environment/Deamons
License:  BSDv3
Url:      http://zimg.buaa.us
Packager: zp@buaa.us
Source:   %{name}-%{version}.tar.gz
BuildRoot:%{_tmppath}/%{name}-%{version}-%{release}-root
Requires: openssl-devel,libevent-devel >= 2.0.20,libjpeg-devel,giflib-devel,libpng-devel,libwebp-devel,ImageMagick-devel,libmemcached-devel >= 1.0.8
Prefix:   %{_prefix}
Prefix:   %{_sysconfdir}
%define   zimgpath /usr/local/zimg

%description
Project zimg's rpm suite.

%prep
%setup -c

%install
install -d $RPM_BUILD_ROOT%{zimgpath}
cp -a %{name}-%{version}/* $RPM_BUILD_ROOT%{zimgpath}
chmod -R 777 $RPM_BUILD_ROOT%{zimgpath}

%postun
rm -rf $RPM_BUILD_ROOT%{zimgpath}

%clean
rm -rf $RPM_BUILD_ROOT
rm -rf $RPM_BUILD_DIR/%{name}-%{version}

%files
%defattr(-,root,root)
%{zimgpath}
