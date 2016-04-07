#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <float.h>
#include <maxminddb.h>
#include "vmod_geo.h"

#ifndef DEBUG
#define DEBUG 1
#endif

static char*  MMDB_CITY_PATH = MAX_CITY_DB;
static char*  DEFAULT_WEATHER_CODE = "New YorkNYUS";
static char*  DEFAULT_LOCATION = "{\"city\":\"New York\",\"state\":\"NY\",\"country\":\"US\"}";
static char*  DEFAULT_TIMEZONE = "{\"timezone\":\"America/New_York\"}";

// close gets called by varnish when then the treads destroyed
void close_mmdb(void *mmdb_handle)
{
    // don't do anything if the db didn't open correctly.
    if (mmdb_handle == NULL) {
        return;
    }
    MMDB_s *handle = (MMDB_s *)mmdb_handle;
    MMDB_close(handle);
}

// Open the maxmind db file
int
open_mmdb(MMDB_s *mmdb_handle)
{
    int mmdb_baddb = MMDB_open(MMDB_CITY_PATH, MMDB_MODE_MMAP, mmdb_handle);
    if (mmdb_baddb != MMDB_SUCCESS) {
        #if DEBUG
        fprintf(stderr, "[ERROR] open_mmdb: Can't open %s - %s\n",
            MMDB_CITY_PATH, MMDB_strerror(mmdb_baddb));
        if (MMDB_IO_ERROR == mmdb_baddb) {
            fprintf(stderr,
                "[ERROR] open_mmdb: IO error: %s\n",
                strerror(mmdb_baddb));
        }
        #endif
        mmdb_handle = NULL;
        return 1;
    }
    return 0;
}

// lookup an ip address using the maxmind db and return the value
// lookup_path described in this doc: http://maxmind.github.io/MaxMind-DB/
int
geo_lookup(MMDB_s *const mmdb_handle, 
	   const char *ipstr, 
	   const char **lookup_path, 
	   char *data, 
	   unsigned max_len)
{
    // Lookup IP in the DB
    int gai_error, mmdb_error;

    MMDB_lookup_result_s result =
        MMDB_lookup_string(mmdb_handle, ipstr, &gai_error, &mmdb_error);

    if (0 != gai_error) {
        #if DEBUG
        fprintf(stderr,
            "[INFO] Error from getaddrinfo for %s - %s\n\n",
            ipstr, gai_strerror(gai_error));
        #endif
        return 0;
    }

    if (MMDB_SUCCESS != mmdb_error) {
        #if DEBUG
        fprintf(stderr,
            "[ERROR] Got an error from libmaxminddb: %s\n\n",
            MMDB_strerror(mmdb_error));
        #endif
        return 0;
    }

    // Parse results
    MMDB_entry_data_s entry_data;
    int exit_code = 0;

    if (result.found_entry) {
        int status = MMDB_aget_value(&result.entry, &entry_data, lookup_path);

        if (MMDB_SUCCESS != status) {
            #if DEBUG
            fprintf(
                stderr,
                    "[WARN] Got an error looking up the entry data. Make sure \
 the lookup_path is correct. %s\n",
                    MMDB_strerror(status));
            #endif
            exit_code = 4;
            return 0;
        }

        if (entry_data.has_data) {

            switch(entry_data.type) {
            case MMDB_DATA_TYPE_UTF8_STRING: {
		if (entry_data.data_size <= max_len) {
		  strncpy(data, entry_data.utf8_string, (size_t)entry_data.data_size);
		  data[entry_data.data_size] = '\0'; // the data we're copying doesn't have nulls
		}
		return entry_data.data_size;
            }
            case MMDB_DATA_TYPE_UINT16: {
                uint16_t num = UINT16_MAX;
                int len      = (int)((ceil(log10(num)))*sizeof(char));
                len++;
		if (len < max_len) {
		  snprintf(data, len, "%u", entry_data.uint16);
                  data[len] = '\0';
		}
                return len;
            }
            default:
                #if DEBUG
                fprintf(
                    stderr,
                        "[WARN] No handler for entry data type (%d) was found\n",
                        entry_data.type);
                #endif
                exit_code = 6;
                return 0;
            }
        } else {
            return 0;
        }
    } else {
        #if DEBUG
        fprintf(
            stderr,
                "[INFO] No entry for this IP address (%s) was found\n",
                ipstr);
        #endif
        exit_code = 5;
        return 0;
    }
    return 0;
}

// Given a valid result and some entry data, lookup a value
// NOTE: You must free() the return value if != NULL
//
// @result - pointer to a result after calling MMDB_lookup_string
// @path - lookup value for MMDB_aget_value
// @return - NULL on failure
char *
get_value(MMDB_lookup_result_s *result, const char **path)
{
    MMDB_entry_data_s entry_data;
    int status  = MMDB_aget_value( &(*result).entry, &entry_data, path);
    if (MMDB_SUCCESS != status) {
        #if DEBUG
        fprintf(
            stderr,
                "[WARN] get_value got an error looking up the entry data. Make sure you use the correct path - %s\n",
                MMDB_strerror(status));
        #endif
        return NULL;
    }
    char *value = NULL;
    if (entry_data.has_data) {
        switch(entry_data.type) {
        case MMDB_DATA_TYPE_UTF8_STRING:
            value = strndup(entry_data.utf8_string, entry_data.data_size);
            break;
        case MMDB_DATA_TYPE_UINT16: {
            uint16_t num = UINT16_MAX;
            int len      = (int)((ceil(log10(num)))*sizeof(char));
            value        = calloc(sizeof(char), len+1);
            snprintf(value, len, "%u", entry_data.uint16);
            break;
        }
        case MMDB_DATA_TYPE_DOUBLE: {
            double num = DBL_MAX;
            int len    = (int)((ceil(log10(num)))*sizeof(char));
            value      = calloc(sizeof(char), len+1);
            snprintf(value, len, "%f", entry_data.double_value);
            break;
        }
        case MMDB_DATA_TYPE_BOOLEAN:
            // i'm assuming true == 1 and false == 0
            value   = calloc(sizeof(char), 2);
            snprintf(value, 1, "%d", entry_data.boolean);
            break;
        default:
            #if DEBUG
            fprintf(
                stderr,
                    "[WARN] get_value: No handler for entry data type (%d) was found. \n",
                    entry_data.type);
            #endif
            break;
        }
    }
    return value;
}

// Given a valid result and some entry data, lookup a value and write to write_to, no more than max_len chars
int
set_value(MMDB_lookup_result_s *result, const char **path, char *write_to, unsigned max_len)
{
    unsigned wrote = 0;
    MMDB_entry_data_s entry_data;
    int status  = MMDB_aget_value( &(*result).entry, &entry_data, path);
    if (MMDB_SUCCESS != status) {
        #if DEBUG
        fprintf(
            stderr,
                "[WARN] get_value got an error looking up the entry data. Make sure you use the correct path - %s\n",
                MMDB_strerror(status));
        #endif
        return wrote;
    }
    
    if (entry_data.has_data) {
        switch(entry_data.type) {
        case MMDB_DATA_TYPE_UTF8_STRING: {
            int len = (entry_data.data_size+1);
            if (len < max_len) {
                snprintf(write_to, len, "%s", entry_data.utf8_string); // +1 to write the \0
                wrote = len;
            }
            break;
        }
        case MMDB_DATA_TYPE_UINT16: {
            uint16_t num = UINT16_MAX;
            int len      = (int)((ceil(log10(num)))*sizeof(char));
            len++;
            if (len < max_len) {
                snprintf(write_to, len, "%u", entry_data.uint16);
                wrote = len;
            }
            break;
        }
        case MMDB_DATA_TYPE_DOUBLE: {
            double num = DBL_MAX;
            int len    = (int)((ceil(log10(num)))*sizeof(char));
            len++;
            if (len < max_len) {
                snprintf(write_to, len, "%f", entry_data.double_value);
                wrote = len;
            }
            break;
        }
        case MMDB_DATA_TYPE_BOOLEAN:
            // i'm assuming true == 1 and false == 0
            if (2 < max_len) {
                snprintf(write_to, 2, "%d", entry_data.boolean);
                wrote = 2;
            }
            break;
        default:
            #if DEBUG
            fprintf(
                stderr,
                    "[WARN] get_value: No handler for entry data type (%d) was found. \n",
                    entry_data.type);
            #endif
            break;
        }
    }
    return wrote;
}

int
geo_lookup_location(MMDB_s *const mmdb_handle,
                    const char *ipstr,
                    int use_default,
                    char *data,
                    unsigned max_len)
{
    if (mmdb_handle == NULL || ipstr == NULL) {
        fprintf(stderr, "[WARN] geo vmod given NULL maxmind db handle");
        return copy(data, DEFAULT_LOCATION, max_len);
    }

    // Lookup IP in the DB
    int ip_lookup_failed, db_status;
    MMDB_lookup_result_s result =
        MMDB_lookup_string(mmdb_handle, ipstr, &ip_lookup_failed, &db_status);

    if (ip_lookup_failed) {
        #if DEBUG
        fprintf(stderr,
            "[WARN] vmod_lookup_location: Error from getaddrinfo for IP: %s Error Message: %s\n",
            ipstr, gai_strerror(ip_lookup_failed));
        #endif
        // we don't want null, if we're not using default
        if (use_default) {
            return copy(data, DEFAULT_LOCATION, max_len);
        } else {
            return copy(data, "--", max_len);
        }
    }

    if (db_status != MMDB_SUCCESS) {
        #if DEBUG
        fprintf(stderr,
            "[ERROR] vmod_lookup_location: libmaxminddb failure. \
Maybe there is something wrong with the file: %s libmaxmind error: %s\n",
            MMDB_CITY_PATH,
            MMDB_strerror(db_status));
        #endif
        if (use_default) {
            return copy(data, DEFAULT_LOCATION, max_len);
        } else {
            return copy(data, "--", max_len);
        }
    }

    // these varaibles will hold our results
    const int MAX = 255;
    char country[MAX];
    char city[MAX];
    char state[MAX];

    // these are used to extract values from the mmdb
    const char *country_lookup[] = {"country", "iso_code", NULL};
    const char *city_lookup[]    = {"city", "names", "en", NULL};
    const char *state_lookup[]   = {"subdivisions", "0", "iso_code", NULL};

    if (result.found_entry) {
        int wrote;
        wrote = set_value(&result, country_lookup, country, MAX);
        wrote = set_value(&result, city_lookup, city, MAX);

        if (country != NULL && strcmp(country,"US") == 0) {
            wrote = set_value(&result, state_lookup, state, MAX);
        } else {
            state[0] = '\0';
        }

        // we should always return new york
        if (country == NULL || city == NULL || state == NULL) {

            if (use_default) {
                return copy(data, DEFAULT_LOCATION, max_len);
            } else {
                if (country == NULL) {
                    country[0] = '\0';
                }
                if (city == NULL) {
                    city[0] = '\0';
                }
                if (state == NULL) {
                    state[0] = '\0';
                }
                size_t len = (sizeof(char) * (strlen(country) + strlen(city) + strlen(state)) );
                const char* fmt = "{\"city\":\"%s\",\"state\":\"%s\",\"country\":\"%s\"}";
                len += (sizeof(char) * strlen(fmt));
                len -= (sizeof(char) * 6);
                len++;
                if (len < max_len) {
                    snprintf(data, len, fmt, city, state, country);
                    return len;
                } else {
                    if (use_default) {
                        return copy(data, DEFAULT_LOCATION, max_len);
                    } else {
                        return copy(data, "--", max_len);
                    }
                }
            }
        } else {
            size_t len = (sizeof(char)* (strlen(country) + strlen(city) + strlen(state)));
            const char* fmt = "{\"city\":\"%s\",\"state\":\"%s\",\"country\":\"%s\"}";
            
            len += sizeof(char) * strlen(fmt);
            len -= sizeof(char) * 6; // reduce by the number of %s
            len++; // add for null
            if (len < max_len) {
                snprintf(data, len, fmt, city, state, country);
                return len;
            } else {
                if (use_default) {
                    return copy(data, DEFAULT_LOCATION, max_len);
                } else {
                    return copy(data, "--", max_len);
                }
            }
        }

    } else {
        #if DEBUG
        fprintf(
            stderr,
                "[INFO] No entry for this IP address (%s) was found\n",
                ipstr);
        #endif
        return copy(data, DEFAULT_LOCATION, max_len);
    }
}

int
geo_lookup_timezone(MMDB_s *const mmdb_handle, const char *ipstr, int use_default, char *data, unsigned max_len)
{
    if (mmdb_handle == NULL || ipstr == NULL) {
        fprintf(stderr, "[WARN] geo vmod given NULL maxmind db handle");
        return copy(data, DEFAULT_TIMEZONE, max_len);
    }

    // Lookup IP in the DB
    int ip_lookup_failed, db_status;
    MMDB_lookup_result_s result =
        MMDB_lookup_string(mmdb_handle, ipstr, &ip_lookup_failed, &db_status);

    if (ip_lookup_failed) {
        #if DEBUG
        fprintf(stderr,
            "[WARN] vmod_lookup_location: Error from getaddrinfo for IP: %s Error Message: %s\n",
            ipstr, gai_strerror(ip_lookup_failed));
        #endif
        // we don't want null, if we're not using default
        if (use_default) {
            return copy(data, DEFAULT_TIMEZONE, max_len);
        } else {
            return copy(data, "--", max_len);
        }
    }

    if (db_status != MMDB_SUCCESS) {
        #if DEBUG
        fprintf(stderr,
            "[ERROR] vmod_lookup_location: libmaxminddb failure. \
Maybe there is something wrong with the file: %s libmaxmind error: %s\n",
            MMDB_CITY_PATH,
            MMDB_strerror(db_status));
        #endif
        if (use_default) {
            return copy(data, DEFAULT_TIMEZONE, max_len);
        } else {
            return copy(data, "--", max_len);
        }
    }

    // these varaibles will hold our results
    int MAX = 255;
    char timezone[MAX];

    // these are used to extract values from the mmdb
    const char *timezone_lookup[] = {"location", "time_zone", NULL};
    
    if (result.found_entry) {
        // we should always return new york
        if (set_value(&result, timezone_lookup, timezone, MAX) == 0) {
            return copy(data, DEFAULT_TIMEZONE, max_len);
        } else {
            size_t len = (sizeof(char)* ( strlen(timezone)) ) + 1;
            const char *txt = "{\"timezone\":\"%s\"}";
            len += (sizeof(char) * (strlen(txt)-2));
            len++; // add null for snprintf
            if (len < max_len) {
                snprintf(data, len, txt, timezone);
                return len;
            } else {
                return copy(data, DEFAULT_TIMEZONE, max_len);
            }
        }
    } else {
        #if DEBUG
        fprintf(
            stderr,
                "[INFO] No entry for this IP address (%s) was found\n",
                ipstr);
        #endif
        return copy(data, DEFAULT_TIMEZONE, max_len);
    }

}

// This function builds up a code we need to lookup weather
// using Accuweather data.
// country code                     e.g. US
// city                             e.g. Beverly Hills
// if country code == US, get region e.g. CA
// And then return "Beverly HillsCAUS" if a US address or
//                    "Paris--FR" if non US
int
geo_lookup_weather(MMDB_s *const mmdb_handle,
                   const char *ipstr,
                   int use_default,
                   char *data,
                   unsigned max_len)
{
    if (mmdb_handle == NULL) {
        fprintf(stderr, "[WARN] geo vmod given NULL maxmind db handle");
        return copy(data, DEFAULT_WEATHER_CODE, max_len);
    }

    // Lookup IP in the DB
    int ip_lookup_failed, db_status;
    MMDB_lookup_result_s result =
        MMDB_lookup_string(mmdb_handle, ipstr, &ip_lookup_failed, &db_status);

    if (ip_lookup_failed) {
        #if DEBUG
        fprintf(stderr,
            "[WARN] vmod_lookup_weathercode: Error from getaddrinfo for IP: %s Error Message: %s\n",
            ipstr, gai_strerror(ip_lookup_failed));
        #endif
        // we don't want null, if we're not using default
        if (use_default) {
            return copy(data, DEFAULT_WEATHER_CODE, max_len);
        } else {
            return copy(data, "--", max_len);
        }
    }

    if (db_status != MMDB_SUCCESS) {
        #if DEBUG
        fprintf(stderr,
            "[ERROR] vmod_lookup_weathercode: libmaxminddb failure. \
Maybe there is something wrong with the file: %s libmaxmind error: %s\n",
            MMDB_CITY_PATH,
            MMDB_strerror(db_status));
        #endif
        if (use_default) {
            return copy(data, DEFAULT_WEATHER_CODE, max_len);
        } else {
            return copy(data, "--", max_len);
        }
    }

    // these varaibles will hold our results
    const int MAX = 255;
    char country[MAX];
    char city[MAX];
    char state[MAX];

    // these are used to extract values from the mmdb
    const char *country_lookup[] = {"country", "iso_code", NULL};
    const char *city_lookup[]    = {"city", "names", "en", NULL};
    const char *state_lookup[]   = {"subdivisions", "0", "iso_code", NULL};

    if (result.found_entry) {
        set_value(&result, city_lookup, city, MAX);
        set_value(&result, country_lookup, country, MAX);

        if (country != NULL && strcmp(country,"US") == 0) {
            set_value(&result, state_lookup, state, MAX);
        } else {
            copy(state, "--", MAX);
        }

        // we should always return new york
        if (country == NULL || city == NULL || state == NULL) {

            if (use_default) {
                return copy(data, DEFAULT_WEATHER_CODE, max_len);
            } else {
                if (country == NULL) {
                    copy(country, "--", MAX);
                }
                if (city == NULL) {
                    copy(city, "--", MAX);
                }
                if (state == NULL) {
                    copy(state, "--", MAX);
                }
                size_t len = (sizeof(char) * (strlen(country) + strlen(city) + strlen(state)) ) + 1;
                len++;
                if (len < max_len) {
                    snprintf(data, len, "%s%s%s", city, state, country);
                    return len;
                } else {
                    return copy(data, DEFAULT_WEATHER_CODE, max_len);
                }
            }
        } else {
            size_t len = (sizeof(char)* ( strlen(country) + strlen(city) + strlen(state)) ) + 1;
            len++;
            if (len < max_len) {
                snprintf(data, len, "%s%s%s", city, state, country);
                return len;
            } else {
                return copy(data, DEFAULT_WEATHER_CODE, max_len);
            }
        }

    } else {
        #if DEBUG
        fprintf(
            stderr,
                "[INFO] No entry for this IP address (%s) was found\n",
                ipstr);
        #endif
        return copy(data, DEFAULT_WEATHER_CODE, max_len);
    }
}



// a utility function for doing large scale testing
void
dump_failed_lookup(MMDB_s *const mmdb_handle, const char *ipstr, const char *outputfile)
{
    if (mmdb_handle == NULL) {
        fprintf(stderr, "[WARN] geo vmod given NULL maxmind db handle");
        return;
    }

    // Lookup IP in the DB
    int ip_lookup_failed, db_status;
    MMDB_lookup_result_s result =
        MMDB_lookup_string(mmdb_handle, ipstr, &ip_lookup_failed, &db_status);

    if (ip_lookup_failed) {
        #if DEBUG
        fprintf(stderr,
            "[WARN] vmod_lookup_weathercode: Error from getaddrinfo for IP: %s Error Message: %s\n",
            ipstr, gai_strerror(ip_lookup_failed));
        #endif
        return;
    }

    if (db_status != MMDB_SUCCESS) {
        #if DEBUG
        fprintf(stderr,
            "[ERROR] vmod_lookup_weathercode: libmaxminddb failure. \
Maybe there is something wrong with the file: %s libmaxmind error: %s\n",
            MMDB_CITY_PATH,
            MMDB_strerror(db_status));
        #endif
        return;
    }

    FILE *f = fopen(outputfile, "a+");

    if (f == NULL) {
        #if DEBUG
        fprintf(stderr, "[ERROR] Unable to open the output file %s\n", outputfile);
        #endif
        return;
    }

    MMDB_entry_data_list_s *entry_data_list = NULL;
    int status = MMDB_get_entry_data_list(&result.entry,
        &entry_data_list);

    if (MMDB_SUCCESS != status) {
        #ifndef DEBUG
        fprintf(
            stderr,
                "Got an error looking up the entry data - %s\n",
                MMDB_strerror(status));
        #endif
        return;
    }

    if (entry_data_list != NULL) {
        const char *proxy_lookup[] = {"traits", "is_anonymous_proxy", NULL};
        const char *trait_lookup[] = {"traits", "is_satellite_provider", NULL};
        const char *lat_lookup[] = {"location", "latitude", NULL};
        const char *lon_lookup[] = {"location", "longitude", NULL};
        char *lat = get_value(&result, lat_lookup);
        char *lon = get_value(&result, lon_lookup);
        char *proxy = get_value(&result, proxy_lookup);
        char *satellite = get_value(&result, trait_lookup);
        if ( (proxy != NULL &&
                strcmp(proxy,"true")) ||
            (satellite != NULL && strcmp(satellite,"true"))) {
            // we don't care about this.
            char *proxy_satelitte = (proxy == NULL) ? "satellite" : "proxy";
            fprintf(f,"%s,%s\n",ipstr, proxy_satelitte);
        } else if (lat != NULL && lon != NULL) {
            fprintf(f,"%s,%s,%s\n",ipstr,lat,lon);
        } else {
            // macro this out. If you need to debug and output all that comes back for
            // a given lookup, this can be re-enabled.
#ifdef DONTRUN
            const char *reg_lookup[] = {"registered_country", "iso_code", NULL};
            char *reg_country = get_value(&result, reg_lookup);
            if (reg_country != NULL) {
                fprintf(f, "%s,%s\n", ipstr, reg_country);
            } else {
                size_t len = (sizeof(char) * strlen(ipstr)+50);
                stuff = calloc( len, sizeof(char) );
                fprintf(f, "{\"%s\":", ipstr);
                MMDB_dump_entry_data_list(f, entry_data_list, 2);
                fprintf(f, "},");
            }
#endif
        }
    }
    fclose(f);
}

// we only want the first part of the cookie, up to the |
int
get_weather_code_from_cookie(const char *cookiestr, const char *cookiename, char *data, unsigned max_len)
{
    max_len = get_cookie(cookiestr, cookiename, data, max_len);
    if (max_len > 0) {
        char* sep = strstr(data, "|");
        if (sep != NULL) {
            *sep = '\0'; // swap the | for a nul, to terminate
            max_len = (sep - data); // reduces the amount of memory we need
        }
    }
    return max_len;
}


int
get_cookie(const char *cookiestr, const char *cookiename, char *data, unsigned max_len)
{
    const char *found = cookiestr;

    do {
        found = strstr(found, cookiename);

        if (found == NULL) {
            return 0;
        }

        found += strlen(cookiename);

        // next character has to be equal or space
        if (*found == ' ' || *found == '=') {
            break;
        }

    } while(found);

    // cookies can have white space after the name, before the =
    while (*found && *found != '=') {
        ++found; // move past the = sign
    }

    // should be at equal at this point
    if (*found != '=') {
        return 0;
    }
    ++found;

    // we should not have any white space after the = symbol
    // and if the next char is a ; there is no value for the cookie
    if (*found == '\0' || *found == ';' || *found == ' ') {
        return 0;
    }

    // find the end of the cookie. cookies are name=value;
    char* end = (char *)found;
    while (*end && *end != ';' && *end != ' ') {
        ++end;
    }

    int len      = (end - found);
    len++;
    if (len < max_len) {
        snprintf(data, len, "%s", found);
        data[len] = '\0';
        return len;
    }
    
    return 0;
}

int
copy(char *dest, const char *src, unsigned max_len)
{
    unsigned len = strlen(src);
    ++len;
    if (len < max_len) {
        strncpy(dest, src, len);
        dest[len] = '\0';
        return len;
    } else {
        dest = NULL;
    }
    return 0;
}
