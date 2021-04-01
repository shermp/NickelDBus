#include <dlfcn.h>
#include <NickelHook.h>
#include "util.h"

namespace NDB {

void ndbResolveSymbol(void* libnickel, const char *name, void **fn) {
    if (!(*fn = dlsym(libnickel, name))) {
        nh_log("info... could not load %s", name);
    }
}

void ndbResolveSymbolRTLD(const char *name, void **fn) {
    if (!(*fn = dlsym(RTLD_DEFAULT, name))) {
        nh_log("info... could not load %s", name);
    }
}

} // namespace
