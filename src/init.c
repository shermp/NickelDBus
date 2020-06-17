#define _GNU_SOURCE // program_invocation_short_name
#include <dlfcn.h>
#include <errno.h> // program_invocation_short_name
#include <link.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "nm/failsafe.h"
// Because the failsafe uses it, we may as well too
#include "nm/util.h"

#ifndef NDB_VERSION
    #define NDB_VERSION "dev"
#endif
#ifndef NM_CONFIG_DIR
    #define NM_CONFIG_DIR "/mnt/onboard/.adds/ndb"
#endif

__attribute__((constructor)) void ndb_init() {
    char *err;

    NM_LOG("version: " NDB_VERSION);
    NM_LOG("config dir: %s", NM_CONFIG_DIR);

    NM_LOG("init: creating failsafe");
    nm_failsafe_t *fs;
    if (!(fs = nm_failsafe_create(&err)) && err) {
        NM_LOG("error: could not create failsafe: %s, stopping", err);
        free(err);
        goto stop;
    }

    NM_LOG("init: checking for uninstall flag");
    if (!access(NM_CONFIG_DIR "/uninstall", F_OK)) {
        NM_LOG("init: flag found, uninstalling");
        nm_failsafe_uninstall(fs);
        unlink(NM_CONFIG_DIR "/uninstall");
        goto stop;
    }


    NM_LOG("init: destroying failsafe");
    nm_failsafe_destroy(fs, 5);

stop:
    NM_LOG("init: done");
    return;
}