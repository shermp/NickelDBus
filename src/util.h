#include <stdio.h>
#include <syslog.h>

#ifndef NH_VERSION
    #define NH_VERSION "dev"
#endif

// Log a message and return 'ret' if cond == fail
#define NDB_ASSERT(ret, cond, fmt, ...) if (!(cond)) { \
    nh_log(fmt, ##__VA_ARGS__);                       \
    return ret;}

// like NDB_ASSERT, but sends an appropriate d-bus error on the bus before returning
// Note: make sure you are including <QtDBus> and <QDBusContext> for this to work!
#define NDB_DBUS_ASSERT(dbus_err, cond, fmt, ...) if (!(cond)) {      \
    nh_log(fmt, ##__VA_ARGS__);                                      \
    if (calledFromDBus()) {                                           \
        QString qstr;                                                 \
        sendErrorReply((dbus_err), qstr.sprintf(fmt, ##__VA_ARGS__)); \
    }                                                                 \
    return NDB_DBUS_RETERR;                                           \
}

// Shorthand for the common USBMS assertion
#define NDB_DBUS_USB_ASSERT() NDB_DBUS_ASSERT(QDBusError::InternalError, !ndbInUSBMS(), "not calling method %s: in usbms session", __func__)