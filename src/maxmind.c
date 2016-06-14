#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <maxminddb.h>
#include "vmod_geo.h"

/*
 * This is a simple tool to validate a maxmind db file. We test validity by opening
 * the maxmind db file and then verifying 4.4.4.4 comes back with US as the country.
 * That may not be valid in the future, but it's currently one of Googles name
 * servers.
 */

void usage();
void usage() {
    fprintf(stdout, "maxmind:\n");
    fprintf(stdout, "-m <path to maxmind db file> : location to valid maxmind database file.\n");
}

char * MMDB_PATH = NULL;
int verbose = 0;

int main(int argc, char **argv) {
    int c, errno = 0;

    while ((c = getopt (argc, argv, "m:v")) != -1) {
        switch (c) {
        case 'm' :
            MMDB_PATH = optarg;
            break;
        case 'v' :
            verbose = 1;
            break;
        case '?' :
            if (optopt == 'm') {
                fprintf(stderr, "Option -%c requires you give the location of the file to use.\n", optopt);
            } else if (isprint (optopt)) {
               fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            } else {
               fprintf (stderr,
                        "Unknown option character `\\x%x'.\n",
                        optopt);
            }
        default :
            return 1;
        }
    }

    if (MMDB_PATH == NULL) {
        usage();
        return 1;
    }

    MMDB_s mmdb_handle;
    int mmdb_baddb = MMDB_open(MMDB_PATH, MMDB_MODE_MMAP, &mmdb_handle);
    if (mmdb_baddb) {
        if (verbose) {
            fprintf(stderr, "Invalid maxmind db file: %s\n", MMDB_PATH);
        }
        return 1;
    }

    const char *country_lookup[] = {"country", "iso_code", NULL};
    const char *expected = "US";
    char *actual = geo_lookup(&mmdb_handle, "4.4.4.4", country_lookup);
    if (strncmp(actual, expected, 2)) {
        if (verbose) {
            fprintf(stderr, "Invalid mamxind db file. Bad actual for 4.4.4.4 - should be 'US' but was '%s'\n", actual);
        }
        return 1;
    }
    MMDB_close(&mmdb_handle);
    if (verbose) {
        fprintf(stdout, "maxmind db file valid.\n");
    }
    return 0;
}
