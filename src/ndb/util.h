#include <stdio.h>
#include <syslog.h>

#ifndef NH_VERSION
    #define NH_VERSION "dev"
#endif

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