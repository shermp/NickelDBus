#ifndef _GNU_SOURCE
#define _GNU_SOURCE // program_invocation_short_name
#endif
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h> // program_invocation_short_name
#include <link.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "semver_c/semver.h"

#include "../NickelMenu/src/failsafe.h"
// Because the failsafe uses it, we may as well too
#include "nickel_dbus.h"
#include "util.h"

#ifndef NDB_CONFIG_DIR
    #define NDB_CONFIG_DIR "/mnt/onboard/.adds/ndb"
#endif
#ifndef NDB_DBUS_CFG_PATH
    #define NDB_DBUS_CFG_PATH "/etc/dbus-1/system.d/local-shermp-nickeldbus.conf"
#endif

NickelDBus *ndb;
static const char ndb_version[] = NDB_VERSION;
static const char ndb_so_ext[] = ".so";
static const char ndb_failsafe_ext[] = ".failsafe";

// This is a quick and dirty alternative to goto style cleanup, which can't be used because C++
#define NDB_INIT_STOP NDB_LOG("init: done"); return
#define NDB_INIT_FS_STOP NDB_LOG("init: destroying failsafe"); nm_failsafe_destroy(fs, 5); NDB_LOG("init: done"); return

static char* ndb_extract_semver_from_fn(const char* fn, char* semver, size_t semver_size) {
    while (*fn) {
        if (*fn >= '0' && *fn <= '9') {
            break;
        }
        fn++;
    }
    // No digits were found, no valid semver in here
    if (*fn <= '0' || *fn >= '9') {
        return nullptr;
    }
    if (strlen(fn) < semver_size) {
        strcpy(semver, fn);
    } else {
        return nullptr;
    }
    // get rid of those pesky file extensions...
    char *ext_begin;
    if ((ext_begin = strrchr(semver, '.')) && !strcmp(ext_begin, ndb_failsafe_ext)) {
        *ext_begin = '\0';
    }
    if ((ext_begin = strrchr(semver, '.')) && !strcmp(ext_begin, ndb_so_ext)) {
        *ext_begin = '\0';
    }
    return semver;
}

__attribute__((constructor)) void ndb_init() {
    char *err;
    NDB_LOG("version: " NDB_VERSION);
    NDB_LOG("config dir: %s", NDB_CONFIG_DIR);

    NDB_LOG("init: creating failsafe");
    nm_failsafe_t *fs;
    if (!(fs = nm_failsafe_create(&err)) && err) {
        NDB_LOG("error: could not create failsafe: %s, stopping", err);
        free(err);
        NDB_INIT_STOP;
    }

    NDB_LOG("init: checking for uninstall flag");
    if (!access(NDB_CONFIG_DIR "/uninstall", F_OK)) {
        NDB_LOG("init: flag found, uninstalling");
        nm_failsafe_uninstall(fs);
        unlink(NDB_CONFIG_DIR "/uninstall");
        unlink(NDB_CONFIG_DIR "/bin/ndb-cli");
        unlink(NDB_CONFIG_DIR "/readme.txt");
        rmdir(NDB_CONFIG_DIR "/bin");
        rmdir(NDB_CONFIG_DIR);
        unlink(NDB_DBUS_CFG_PATH);
        NDB_INIT_STOP;
    }
    NDB_LOG("Init: Checking version");
    const char libndb_pre[] = "libndb";
    char semver_buff[256] = {};
    char *sv;
    DIR *dir;
    struct dirent *de;
    if (!(dir = opendir("/usr/local/Kobo/imageformats"))) {
        NDB_LOG("error: could not open /usr/local/Kobo/imageformats");
        NDB_INIT_STOP;
    }
    semver_t curr_vers;
    semver_t comp_vers;
    if (semver_parse(ndb_version, &curr_vers) != 0) {
        NDB_LOG("error: current version is not a valid semantic version string");
        NDB_INIT_STOP;
    }
    while ((de = readdir(dir)) != nullptr) {
        if (de->d_type == DT_REG && !(strncmp(libndb_pre, de->d_name, sizeof(libndb_pre) - 1))) {
            if ((sv = ndb_extract_semver_from_fn(de->d_name, semver_buff, sizeof(semver_buff)))) {
                if (semver_parse(sv, &comp_vers) != 0) {
                    NDB_LOG("error: %s does not contain a valid semantic version", de->d_name);
                    NDB_INIT_STOP;
                }
                if (semver_compare(curr_vers, comp_vers) < 0) {
                    nm_failsafe_uninstall(fs);
                    NDB_LOG("init: greater version (%s) found. Uninstalling (%s)", sv, ndb_version);
                    NDB_INIT_STOP;
                } else {
                    NDB_LOG("init: current version (%s) is greater than, or equal to (%s)", ndb_version, sv);
                }
            }
        }
    }
    closedir(dir);
    NDB_LOG("init: finished version checking");

    ndb = new NickelDBus(nullptr);
    if (!ndb->initSucceeded) {
        NDB_LOG("init: failed to init NickelDBus object");
        delete ndb;
        NDB_INIT_FS_STOP;
    }
    ndb->connectSignals();

    NDB_INIT_FS_STOP;
}