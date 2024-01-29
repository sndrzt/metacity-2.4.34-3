%define ver 2.4.34
%define RELEASE 1
%define rel %{?CUSTOM_RELEASE} %{!?CUSTOM_RELEASE:%RELEASE}

Summary: Metacity window manager
Name: metacity
Version: %ver
Release: %rel
URL: http://people.redhat.com/~hp/metacity/
Source0: %{name}-%{version}.tar.gz
License: GPL
Group: User Interface/Desktops
BuildRoot: %{_tmppath}/%{name}-root
BuildRequires: gtk2-devel >= 2.0.0
BuildRequires: GConf2-devel >= 1.1.9

%description

Metacity is a simple window manager that integrates nicely with 
GNOME 2.

%prep
%setup -q

%build
%configure
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT

export GCONF_DISABLE_MAKEFILE_SCHEMA_INSTALL=1
%makeinstall
unset GCONF_DISABLE_MAKEFILE_SCHEMA_INSTALL

%clean
rm -rf $RPM_BUILD_ROOT

%post

export GCONF_CONFIG_SOURCE=`gconftool-2 --get-default-source`
SCHEMAS="metacity.schemas"
for S in $SCHEMAS; do
  gconftool-2 --makefile-install-rule %{_sysconfdir}/gconf/schemas/$S > /dev/null
done

%files
%defattr(-,root,root)
%doc README AUTHORS COPYING NEWS HACKING theme-format.txt
%{_bindir}/*
%{_libexecdir}/*
%{_datadir}/gnome/wm-properties/*
%{_sysconfdir}/gconf/schemas/*.schemas
%{_datadir}/control-center-2.0/capplets/*
%{_datadir}/metacity
%{_datadir}/pixmaps/*
%{_datadir}/themes/*

%changelog
* Tue Aug 20 2002 Steve Fox <drfickle@k-lug.org>
- Autoconf-ize the spec file to magic updates
- Include missing dirs

* Thu May  2 2002 Havoc Pennington <hp@redhat.com>
- 2.3.233

* Thu Apr 25 2002 Havoc Pennington <hp@redhat.com>
- rebuild in different environment
- add gconf schemas boilerplate

* Mon Apr 15 2002 Havoc Pennington <hp@pobox.com>
- 2.3.89

* Tue Oct 30 2001 Havoc Pennington <hp@redhat.com>
- 2.3.34

* Fri Oct 13 2001 Havoc Pennington <hp@redhat.com>
- 2.3.21 

* Mon Sep 17 2001 Havoc Pennington <hp@redhat.com>
- 2.3.8
- 2.3.13

* Wed Sep  5 2001 Havoc Pennington <hp@redhat.com>
- Initial build.


