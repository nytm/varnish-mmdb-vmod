#include <stdlib.h>
#include <maxminddb.h>

#include "vrt.h"
#include "bin/varnishd/cache.h"
#include "include/vct.h"
#include "vcc_if.h"
#include "vmod_geo.h"

//**********************************************************************
// This has all of our vmod function definitions. This calls the 
// functions defined in geo_functions.c
//**********************************************************************

// open the maxmind db once, during initialization.
int
init_function(struct vmod_priv *priv, const struct VCL_conf *conf) 
{
    MMDB_s mmdb_handle;
    int mmdb_baddb = open_mmdb(&mmdb_handle);
    if (!mmdb_baddb) {
        priv->priv = (void *)&mmdb_handle;
        priv->free = close_mmdb;
    }
    return mmdb_baddb;
}

// Lookup a field
const char *
vmod_lookup(struct sess *sp, struct vmod_priv *global, const char *ipstr, const char **lookup_path)
{
    const char *data;
    char *cp   = NULL;
    MMDB_s * mmdb_handle = (struct MMDB_s *)global->priv;
    if (mmdb_handle == NULL) {
        fprintf(stderr, "[WARN] varnish gave NULL maxmind db handle");
        return NULL;
    }

    data = geo_lookup(mmdb_handle, ipstr,lookup_path);

    if (data != NULL) {
        cp = WS_Dup(sp->wrk->ws, data);
        free((void *)data);
    }
        
    return cp;
}

// Lookup up a weather code
const char *
vmod_lookup_weathercode(struct sess *sp, struct vmod_priv *global, const char *ipstr)
{
    char *data           = NULL;
    char *cp             = NULL;
    MMDB_s * mmdb_handle = (struct MMDB_s *)global->priv;
    if (mmdb_handle == NULL) {
        fprintf(stderr, "[WARN] varnish gave NULL maxmind db handle");
        return NULL;
    }               
    data = geo_lookup_weather(mmdb_handle, ipstr, 1);
    
    if (data != NULL) {
        cp = WS_Dup(sp->wrk->ws, data);
        free(data);
    }

    return cp;
}

// lookup a city
const char*
vmod_city(struct sess *sp, struct vmod_priv *global, const char *ipstr)
{
    const char *lookup_path[] = {"city", "names", "en", NULL};
    return vmod_lookup(sp, global, ipstr, lookup_path);
}

// lookup a country
const char*
vmod_country(struct sess *sp, struct vmod_priv *global, const char *ipstr)
{
    const char *lookup_path[] = {"country", "names", "en", NULL};
    return vmod_lookup(sp, global, ipstr, lookup_path);
}

// lookup a metro code
const char*
vmod_metro_code(struct sess *sp, struct vmod_priv *global, const char *ipstr)
{
    const char *lookup_path[] = {"location", "metro_code", NULL};
    return vmod_lookup(sp, global, ipstr, lookup_path);
}

// lookup a region
const char*
vmod_region(struct sess *sp, struct vmod_priv *global, const char *ipstr)
{
    const char *lookup_path[] = {"subdivisions", "0", "iso_code", NULL};
    return vmod_lookup(sp, global, ipstr, lookup_path);
}

// lookup a country
const char*
vmod_country_code(struct sess *sp, struct vmod_priv *global, const char *ipstr)
{
    const char *lookup_path[] = {"country", "iso_code", NULL};
    return vmod_lookup(sp, global, ipstr, lookup_path);
}

// lookup an NYT weather code from ip address
const char*
vmod_weather_code(struct sess *sp, struct vmod_priv *global, const char *ipstr)
{
    return vmod_lookup_weathercode(sp, global, ipstr);
}

// get the NYT weather cookie value from the cookie header
const char *
vmod_get_weather_cookie(struct sess *sp, const char *cookiestr, const char *cookiename)
{
    char *data = NULL;
    char *cp   = NULL;
    data       = get_weather_code_from_cookie(cookiestr, cookiename);


    if (data != NULL) {
        cp = WS_Dup(sp->wrk->ws, data);
        free(data);
    }
    
    return cp;
}

// get a cookie value by name from the cookiestr
const char *
vmod_get_cookie(struct sess *sp, const char *cookiestr, const char *cookiename)
{
    char *data = NULL;
    char *cp   = NULL;
    data = get_cookie(cookiestr, cookiename);

    if (data != NULL) {
        cp = WS_Dup(sp->wrk->ws, data);
        free(data);
    }
    return cp;
}
