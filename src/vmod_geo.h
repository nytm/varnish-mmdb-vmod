#include <maxminddb.h>
// can come by way of configure --with-maxminddbfile 
#ifndef MAX_CITY_DB
#define MAX_CITY_DB "/mnt/mmdb/GeoLite2-City.mmdb"
#endif

#ifndef VMOD_GEO_H
#define VMOD_GEO_H

MMDB_s *
get_handle(void);

// function to give to vcl
void 
close_mmdb(void *);

// function to open the maxmind db once
int 
open_mmdb(MMDB_s *);

// function to get a value from the returned mmdb lookup
char *
get_value(MMDB_lookup_result_s *, const char **);

const char *
geo_lookup(const char *ipstr, const char **lookup_path);

void
dump_failed_lookup(const char *ipstr, const char *file_to_write_to);


char *
geo_lookup_weather(const char *ipstr, int use_default);


// the cookie header can be too big for regusub or regusuball so we need
// a function to pull a cookie value from the Cookie header. Here is an
// example of whour cookie can look like:
// NYT_W2=New%20YorkNYUS; abc=ChicagoILUS|London--UK|Los%20AngelesCAUS
// 
// it can be anywere in the cookie string, front, middle blank or not
// even existant. because regsub and regsuball bail when the cookie
// string is larger than 5655, I created this function.
char *
get_weather_code_from_cookie(const char *cookiestr, const char *cookiename);

// get the value of a cookie by name. 
char *
get_cookie(const char *cookiestr, const char *cookiename);

#endif
