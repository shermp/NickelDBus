#include <cstddef>
#include <cstdlib>

#include <NickelHook.h>

#include "ndb.h"

static const char ndb_ininstall_file[] = "/mnt/onboard/.adds/nickeldbus";
static const char ndb_version_str[] = NH_VERSION;

NDBDbus *ndb;

static int ndb_init() {
    // Ensure that the user visible version file has the correct
    // version number.
    if (FILE *f = fopen(ndb_ininstall_file, "w")) {
        nh_log("(init) Writing version to %s", ndb_ininstall_file);
        fwrite(ndb_version_str, sizeof(ndb_version_str) - 1, 1, f);
        fwrite("\n", sizeof(char), 1, f);
        fclose(f);
    } else {
        nh_log("(init) Failed to open %s with error %m", ndb_ininstall_file);
    }

    ndb = new NDBDbus(nullptr);
    if (!ndb->initSucceeded) {
        delete ndb;
        return -1;
    }
    ndb->connectSignals();
    return 0;
}

static bool ndb_uninstall() {
    nh_delete_file("/usr/bin/qndb");
    nh_delete_file("/etc/dbus-1/system.d/com-github-shermp-nickeldbus.conf");
    return true;
}

static struct nh_info NickelDBusInfo = {
    .name            = "NickelDBus",
    .desc            = "Observe and control Nickel over D-Bus",
    .uninstall_flag  = NULL,
    .uninstall_xflag = ndb_ininstall_file
};

static struct nh_hook NickelDBusHook[] = {
    {0},
};

static struct nh_dlsym NickelDBusDlsym[] = {
    {0},
};

NickelHook(
    .init  = ndb_init,
    .info  = &NickelDBusInfo,
    .hook  = NickelDBusHook,
    .dlsym = NickelDBusDlsym,
    .uninstall = ndb_uninstall,
)
