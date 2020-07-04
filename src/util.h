#include <stdio.h>
#include <syslog.h>

#ifndef NDB_LOG_NAME
#ifdef NM_LOG_NAME
#define NDB_LOG_NAME NM_LOG_NAME
#else
#define NDB_LOG_NAME "NickelDBus"
#endif
#endif

#ifndef NDB_VERSION
    #define NDB_VERSION "dev"
#endif

// NDB_LOG writes a log message. Borrowed from NickelMenu
#define NDB_LOG(fmt, ...) syslog(LOG_DEBUG, "(" NDB_LOG_NAME ") " fmt " (%s:%d)", ##__VA_ARGS__, __FILE__, __LINE__)

// Log a message and return 'ret' if cond == fail
#define NDB_ASSERT(ret, cond, fmt, ...) if (!(cond)) { \
    NDB_LOG(fmt, ##__VA_ARGS__);                       \
    return ret;}

// like NDB_ASSERT, but sends an appropriate d-bus error on the bus before returning
// Note: make sure you are including <QtDBus> and <QDBusContext> for this to work!
#define NDB_DBUS_ASSERT(dbus_err, cond, fmt, ...) if (!(cond)) {      \
    NDB_LOG(fmt, ##__VA_ARGS__);                                      \
    if (calledFromDBus()) {                                           \
        QString qstr;                                                 \
        sendErrorReply((dbus_err), qstr.sprintf(fmt, ##__VA_ARGS__)); \
    }                                                                 \
    return NDB_DBUS_RETERR;                                           \
}

// Shorthand for the common USBMS assertion
#define NDB_DBUS_USB_ASSERT(method_name) NDB_DBUS_ASSERT(QDBusError::InternalError, !ndbInUSBMS(), "not calling method %s: in usbms session", (method_name))