Varnish MaxMind GeoIP vmod
--------------------------

**NOTE**
This is for Varnish 3

This vmod provides the ability to lookup location information based on IP address. The GeoIP database is a mmap'd red black tree implementation that's read-only. The code opens the database when the vmod is initialized and shutdown when the thread is terminated, leaving the database open for the life of the thread. The most expensive operation is opening the MaxMind database.

The vmod uses the MaxMind City database. 

## Usage

```
import geo
   // ....
   sub vcl_recv {
     set req.http.X-Country     = geo.country("170.149.100.10")
     set req.http.X-CountryCode = geo.country_code("170.149.100.10")
     set req.http.X-Region      = geo.region("170.149.100.10")
     set req.http.X-MetroCode   = geo.metro_code("170.149.100.10")
     set req.http.X-City        = geo.city("170.149.100.10")
     set req.http.X-Timezone    = geo.timezone("170.149.100.10")
     set req.http.X-Location    = geo.location("170.149.100.10")

     //# nytimes specific stuff
     set req.http.Weather-Code   = geo.weather_code("170.149.100.10")
     set req.http.Weather-Cookie = geo.get_weather_cookie(req.http.Cookie, "NYT_W2")
     set req.http.Cookie-Value   = geo.get_cookie(req.http.Cookie, "NYT_W2")

    }
    // ....
}
```
The location call generates json e.g. geo.location("199.254.0.98") would return

``{"city":"Beverly Hills","state":"CA","country":"US"}"``

## Testing
There are unit tests and varnishtest scripts. The unit tests are in the tests folder. Edit tests/tests.c and rerun make && make test
The unit tests are done with unity https://mark-vandervoord-yxrv.squarespace.com/unity
Varnishtest scripts are in src/tests. To run the tests:

```
make test
```


## Installation

The vmod depends on having the following installed:

* Varnish - https://github.com/varnish/Varnish-Cache
* libmaxminddb - https://github.com/maxmind/libmaxminddb

http://maxmind.github.io/MaxMind-DB provides an indepth look at the MaxMind GeoIP database.

You can get a copy of the city database from here: https://dev.maxmind.com/geoip/geoip2/geolite2/

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
sudo make install
cd ..
```

### Step 3 - build the mddb vmod
```
git clone git@github.com:nytm/varnish-mmdb-vmod.git
cd varnish-mmdb-vmod
./autogen.sh
./configure --prefix=/usr --with-maxminddbfile=/mnt/mmdb/GeoIP2-City.mmdb VARNISHSRC=/usr/local/src/Varnish-Cache VMODDIR=/usr/lib64/varnish/vmods
make
sudo make install
```

**NOTE** I added support for a flag in autoconf:  **--with-maxminddbfile** so that you can decide, when you build the module, where you're data file will live. If you don't specify a value the default will be used **/mnt/mmdb/GeoIP2-City.mmdb** See src/vmod_geo.h

```
#define MAX_CITY_DB "/mnt/mmdb/GeoLite2-City.mmdb"
```

I modified the module to open the maxmind db file once, on Init. If you open the data file with each execution you incur a significant performance impact. 

This will work with the free data or the licensed data. 


