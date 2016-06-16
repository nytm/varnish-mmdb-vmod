%define checkoutName %{name}-%{version}-%{_build_arch}-%{suffix: %{dist}}
%define varnishSrc https://github.com/varnish/Varnish-Cache.git
%define varnishDir VarnishCache-%{_build_arch}
%{!?source: %define source https://github.com/nytm/varnish-mmdb-vmod.git}
%{!?_build_number: %define _build_number 1}

Name:		mmdb-vmod
Version:	1.0.1
Release:	%{_build_number}%{?dist}
Summary:	A varnish module to do IP lookup using libmaxminddb
Group:		Content APIS/DU
License:	Proprietary
URL:		https://github.com/nytm/varnish-mmdb-vmod
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-%{_build_arch}-root-%(%{__id_u} -n)
BuildRequires:	libmaxminddb git

%description
This provides the MaxMind GeoIP feature.

%prep
rm -rf "%{checkoutName}"
if [ ! -d "%{_builddir}/%{varnishDir}" ]; then
  git clone "%{varnishSrc}" "%{varnishDir}"
  cd "%{_builddir}/%{varnishDir}"
  git branch 3.0 -t origin/3.0
  git checkout 3.0
  git checkout 1a89b1f75895bbf874e83cfc6f6123737a3fd76f
  ./autogen.sh
  ./configure --prefix=/usr
  make clean
  make
  cd  "%{_builddir}"
fi

src="%{source}"
if [[ "${src:0:7}" = "file://" ]] ; then :
    cp -R "${src:7}" "%{checkoutName}"
else
    git clone "%{source}" "%{checkoutName}"
    cd "%{checkoutName}"
fi


%build
cd %{checkoutName}
mkdir buildinfo
echo "%{source}" >> buildinfo/source.txt
git branch > buildinfo/branch.txt
git status > buildinfo/status.txt
git remote --verbose show -n origin > buildinfo/origin.txt
git log --max-count=1 --pretty=fuller > buildinfo/log.txt
./autogen.sh
%configure VARNISHSRC=%{_builddir}/%{varnishDir} VMODDIR=%{_libdir}/varnish/vmods --with-maxminddbfile=/mnt/mmdb/GeoIP2-City.mmdb
make clean
make -C tests tests.c
make %{?_smp_mflags}

%debug_package

%install
cd %{checkoutName}
rm -rf %{buildroot}
mkdir %{buildroot}
make install DESTDIR=%{buildroot}
rm -rf $RPM_BUILD_ROOT
mkdir $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT


%clean
#rm -rf $RPM_BUILD_ROOT
#rm -rf %{buildroot}
rm -rf %{checkoutName}

%files
%defattr(-,root,root,-)
/usr/*
%doc



%changelog
