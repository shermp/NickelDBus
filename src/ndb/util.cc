#include <dlfcn.h>
#include <NickelHook.h>
#include "util.h"

void ndbResolveSymbol(void* libnickel, const char *name, void **fn) {
    if (!(*fn = dlsym(libnickel, name))) {
        nh_log("info... could not load %s", name);
    }
}
