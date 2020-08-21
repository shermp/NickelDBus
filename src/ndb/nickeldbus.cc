#include <cstddef>
#include <cstdlib>

#include <NickelHook.h>

#include "ndb.h"

NDB *ndb;

static int ndb_init() {
    ndb = new NDB(nullptr);
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
    .name           = "NickelDBus",
    .desc           = "Observe and control Nickel over D-Bus",
    .uninstall_flag = "/mnt/onboard/ndb_uninstall",
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
