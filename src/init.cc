#ifndef _GNU_SOURCE
#define _GNU_SOURCE // program_invocation_short_name
#endif
#include <dlfcn.h>
#include <errno.h> // program_invocation_short_name
#include <link.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../NickelMenu/src/failsafe.h"
// Because the failsafe uses it, we may as well too
#include "nickel_dbus.h"
#include "util.h"

#ifndef NM_CONFIG_DIR
    #define NM_CONFIG_DIR "/mnt/onboard/.adds/ndb"
#endif

NickelDBus *ndb;

// This is a quick and dirty alternative to goto style cleanup, which can't be used because C++
#define NDB_INIT_STOP NDB_LOG("init: done"); return
#define NDB_INIT_FS_STOP NDB_LOG("init: destroying failsafe"); nm_failsafe_destroy(fs, 5); NDB_LOG("init: done"); return

__attribute__((constructor)) void ndb_init() {
    char *err;
    NDB_LOG("version: " NDB_VERSION);
    NDB_LOG("config dir: %s", NM_CONFIG_DIR);

    NDB_LOG("init: creating failsafe");
    nm_failsafe_t *fs;
    if (!(fs = nm_failsafe_create(&err)) && err) {
        NDB_LOG("error: could not create failsafe: %s, stopping", err);
        free(err);
        NDB_INIT_STOP;
    }

    NDB_LOG("init: checking for uninstall flag");
    if (!access(NM_CONFIG_DIR "/uninstall", F_OK)) {
        NDB_LOG("init: flag found, uninstalling");
        nm_failsafe_uninstall(fs);
        unlink(NM_CONFIG_DIR "/uninstall");
        NDB_INIT_STOP;
    }
    ndb = new NickelDBus(nullptr);
    if (!ndb->dbusRegSucceeded) {
        NDB_LOG("init: failed to register with dbus");
        delete ndb;
        NDB_INIT_FS_STOP;
    }
    // if (!ndb->registerDBus()) {
    //     NM_LOG("init: error registering dbus service");
    //     delete ndb;
    //     NDB_INIT_FS_STOP;
    // }
    ndb->libnickel = dlopen("libnickel.so.1.0.0", RTLD_LAZY|RTLD_NODELETE);
    if (!ndb->libnickel) {
        NDB_LOG("init: could not dlopen libnickel");
        delete ndb;
        NDB_INIT_FS_STOP;
    }
    // Test assertion code
    if (ndb->testAssert(false)) {
        NDB_LOG("The test assert failed");
    }
    ndb->connectSignals();

    NDB_INIT_FS_STOP;
}