#ifndef NDB_UTIL_H
#define NDB_UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

namespace NDB {

#ifndef NH_VERSION
    #define NH_VERSION "dev"
#endif

#define NDB_ASSERT(ret, cond, fmt, ...) if (!(cond)) { nh_log(fmt, ##__VA_ARGS__); return (ret); }
#define NDB_ASSERT_RES(res, func) (res) = (func); if ((res) != Ok) { return (res); }

// like NDB_ASSERT, but sends an appropriate d-bus error on the bus before returning
// Note: make sure you are including <QtDBus> and <QDBusContext> for this to work!
#define NDB_DBUS_ASSERT(ret, dbus_err, cond, fmt, ...) if (!(cond)) { \
    nh_log(fmt, ##__VA_ARGS__);                                       \
    if (calledFromDBus()) {                                           \
        QString qstr;                                                 \
        sendErrorReply((dbus_err), qstr.sprintf(fmt, ##__VA_ARGS__)); \
    }                                                                 \
    return (ret);                                                     \
}

// Shorthand for the common USBMS assertion
#define NDB_DBUS_USB_ASSERT(ret) NDB_DBUS_ASSERT(ret, QDBusError::InternalError, !ndbInUSBMS(), "not calling method %s: in usbms session", __func__)
// Shorthand for the common nickel symbol resolve assertion
#define NDB_DBUS_SYM_ASSERT(ret, cond) NDB_DBUS_ASSERT(ret, QDBusError::InternalError, cond, "%s: required symbol(s) not resolved", __func__)

#define NDB_RESOLVE_SYMBOL(name, fn) resolveSymbol(libnickel, name, fn)

#ifdef DEBUG
#define NDB_DEBUG(fmt, ...) nh_log("[debug] %s:%d:%s() " fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define NDB_DEBUG(fmt, ...) do {} while (0)
#endif

#define ARRAY_LEN(arr) (sizeof((arr)) / sizeof ((arr)[0]))

void resolveSymbol(void* libnickel, const char *name, void **fn);
void resolveSymbolRTLD(const char *name, void **fn);

} // namespace NDB

#endif // NDB_UTIL_H
