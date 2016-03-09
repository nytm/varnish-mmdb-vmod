#Varnish vmod for GeoMaxMind DB
https://www.maxmind.com/en/home

**NOTE**
This is for Varnish 3

I forked this vmod to add new features that would allow me to work with our weather API provider, Accuweather.

## Installation
This module requires the following:

Varnish cache from https://github.com/varnish/Varnish-Cache

libmaxminddb from https://github.com/maxmind/libmaxminddb

http://maxmind.github.io/MaxMind-DB/

Get a copy of the free city database from here: https://dev.maxmind.com/geoip/geoip2/geolite2/
### Step 1 - get the source and build a copy of Varnish


```
cd /usr/local/src
git clone https://github.com/varnish/Varnish-Cache.git
cd Varnish-Cache
git branch 3.0 -t origin/3.0
git checkout 3.0
# make sure i'm matching release versions - 3.0.5 in my case.
git checkout 1a89b1f75895bbf874e83cfc6f6123737a3fd76f
./autogen.sh
./configure --prefix=/usr/local
make
sudo make install
```

**NOTE:** I received the following after running make:

``You need rst2man installed to make dist``

I was able to get past this by installing python-docutils with:
```yum install python-docutils```

I then re-ran everything from ./autogen.sh onward.

### Step 2 - install libmaxmind
```
cd ..
git clone --recursive https://github.com/maxmind/libmaxminddb.git
cd libmaxminddb
git submodule update
./bootstrap
./configure --prefix=/usr/local
make 
make install
cd ..
```
### Step 3 - build the mddb vmod
```
git clone git@github.com:nytm/varnish-mmdb-vmod.git
cd varnish-mmdb-vmod
./autogen.sh
./configure --prefix=/usr --with-maxminddbfile=/mnt/mmdb/GeoIP2-City.mmdb VARNISHSRC=/usr/local/src/Varnish-Cache VMODDIR=/usr/lib64/varnish/vmods
make
make install
```

**NOTE** I added support for a flag in autoconf:  **--with-maxminddbfile** so that you can decide, when you build the module, where you're data file will live. If you don't specify a value the default will be used **/mnt/mmdb/GeoIP2-City.mmdb** See src/vmod_geo.h

```
#define MAX_CITY_DB "/mnt/mmdb/GeoLite2-City.mmdb"
```

I modified the module to open the maxmind db file once, on Init. If you open the data file with each execution you incur a significant performance impact. 

This will work with the free data or the licensed data. 


## vmod usage example
This vmod gives you the following functions:
```
geo.country("170.149.100.10")
geo.country_code("170.149.100.10")
geo.region("170.149.100.10")
geo.metro_code("170.149.100.10")
geo.city("170.149.100.10")

# DU specific stuff
geo.weather_code("170.149.100.10")
geo.get_weather_cookie(req.http.Cookie, "NYT_W2")

# Generic get cookie value from http cookie header
geo.get_cookie(req.http.Cookie, "NYT_W2")


```

Here is some very basic usage:

```
import geo;
import std;

sub vcl_recv{
 set req.http.X-Forwarded-For = client.ip;
 std.syslog(180, geo.city(req.http.X-Forwarded-For));
 std.syslog(180, geo.country(req.http.X-Forwarded-For));
}

```
Here is how I use it:
```
import std;
import geo;
....
set req.http.X-Weather = geo.get_weather_cookie(req.http.Cookie, "NYT_W2");
set req.http.X-Weather = geo.weather_code(req.http.IP);
set req.http.X-Weather = regsuball(req.http.X-Weather, " ", "%20");
```

## Testing
You can add tests in src/tests. Use src/tests/test01.vtc as an example or check out https://github.com/varnish/libvmod-example to see how their done. **NOTE** you will need to 

```
cd src
make tests/*
```
