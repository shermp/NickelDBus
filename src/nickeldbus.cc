#include <cstddef>
#include <cstdlib>

#include <NickelHook.h>

#include "nickel_dbus.h"

NickelDBus *ndb;

static int ndb_init() {
    ndb = new NickelDBus(nullptr);
    if (!ndb->initSucceeded) {
        delete ndb;
        return -1;
    }
    ndb->connectSignals();
    return 0;
}

static struct nh_info NickelDBusInfo = {
    .name           = "NickelDBus",
    .desc           = "Observe and control Nickel over D-Bus",
    .uninstall_flag = "/mnt/onboard/.adds/ndb/ndb_uninstall",
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
)
