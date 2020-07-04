#include <dlfcn.h>
#include <QString>
#include <unistd.h>
#include <string.h>
#include "../NickelMenu/src/action.h"
#include "util.h"
#include "nickel_dbus.h"
#include "adapter/nickel_dbus_adapter.h"

NickelDBus::NickelDBus(QObject* parent) : QObject(parent), QDBusContext() {
    new NickelDBusAdapter(this);
    this->initSucceeded = true;
    if (!conn.registerObject(NDB_DBUS_OBJECT_PATH, this)) {
        NDB_LOG("NickelDBus: failed to register object on system bus");
        this->initSucceeded = false;
        return;
    }
    if (!conn.registerService(NDB_DBUS_IFACE_NAME)) {
        NDB_LOG("NickelDBus: failed to register service on the system bus");
        this->initSucceeded = false;
        return;
    }
    this->libnickel = dlopen("libnickel.so.1.0.0", RTLD_LAZY|RTLD_NODELETE);
    if (!this->libnickel) {
        NDB_LOG("NickelDBus: could not dlopen libnickel");
        initSucceeded = false;
        return;
    }
    reinterpret_cast<void*&>(this->PlugManager__sharedInstance) = dlsym(this->libnickel, "_ZN11PlugManager14sharedInstanceEv");
    if (!this->PlugManager__sharedInstance) {
        NDB_LOG("NickelDBus: could not dlsym PlugManager::sharedInstance");
        initSucceeded = false;
        return;
    }
    reinterpret_cast<void*&>(this->PlugManager__gadgetMode) = dlsym(this->libnickel, "_ZNK11PlugManager10gadgetModeEv");
    if (!this->PlugManager__gadgetMode) {
        NDB_LOG("NickelDBus: could not dlsym PlugManager::gadgetMode");
        initSucceeded = false;
        return;
    }
}
NickelDBus::~NickelDBus() {
    conn.unregisterService(NDB_DBUS_IFACE_NAME);
    conn.unregisterObject(NDB_DBUS_OBJECT_PATH);
}

template <typename T>
void NickelDBus::ndbConnectSignal(T *srcObj, const char *srcSignal, const char *dest) {
    const char *dest_start = dest + 1;
    const char *dest_end = strchr(dest_start, '(');
    NDB_LOG("connecting %s to %s", srcSignal, dest);
    if (QObject::connect(srcObj, srcSignal, this, dest)) {
        connectedSignals.insert(QString::fromLatin1(dest_start, dest_end - dest_start));
    } else {
        NDB_LOG("failed to connect %s to %s", srcSignal, dest);
    }
}
void NickelDBus::connectSignals() {
    PlugWorkflowManager *(*PlugWorkflowManager_sharedInstance)();
    reinterpret_cast<void*&>(PlugWorkflowManager_sharedInstance) = dlsym(this->libnickel, "_ZN19PlugWorkflowManager14sharedInstanceEv");
    if (PlugWorkflowManager_sharedInstance) {
        PlugWorkflowManager *wf = PlugWorkflowManager_sharedInstance();
        if (wf) {
            ndbConnectSignal<PlugWorkflowManager>(wf, SIGNAL(aboutToConnect()), SIGNAL(pfmAboutToConnect()));
            ndbConnectSignal<PlugWorkflowManager>(wf, SIGNAL(doneProcessing()), SIGNAL(pfmDoneProcessing()));
        } else {
            NDB_LOG("could not get shared PlugWorkflowManager pointer");
        }
    } else {
        NDB_LOG("could not dlsym PlugWorkflowManager::sharedInstance");
    }
    WirelessManager *(*WirelesManager_sharedInstance)();
    reinterpret_cast<void*&>(WirelesManager_sharedInstance) = dlsym(this->libnickel, "_ZN15WirelessManager14sharedInstanceEv");
    if (WirelesManager_sharedInstance) {
        WirelessManager *wm = WirelesManager_sharedInstance();
        if (wm) {
            ndbConnectSignal<WirelessManager>(wm, SIGNAL(tryingToConnect()), SIGNAL(wmTryingToConnect()));
            ndbConnectSignal<WirelessManager>(wm, SIGNAL(networkConnected()), SIGNAL(wmNetworkConnected()));
            ndbConnectSignal<WirelessManager>(wm, SIGNAL(networkDisconnected()), SIGNAL(wmNetworkDisconnected()));
            ndbConnectSignal<WirelessManager>(wm, SIGNAL(networkForgotten()), SIGNAL(wmNetworkForgotten()));
            ndbConnectSignal<WirelessManager>(wm, SIGNAL(networkFailedToConnect()), SIGNAL(wmNetworkFailedToConnect()));
            ndbConnectSignal<WirelessManager>(wm, SIGNAL(scanningStarted()), SIGNAL(wmScanningStarted()));
            ndbConnectSignal<WirelessManager>(wm, SIGNAL(scanningFinished()), SIGNAL(wmScanningFinished()));
            ndbConnectSignal<WirelessManager>(wm, SIGNAL(scanningAborted()), SIGNAL(wmScanningAborted()));
            ndbConnectSignal<WirelessManager>(wm, SIGNAL(wifiEnabled(bool)), SIGNAL(wmWifiEnabled(bool)));
            ndbConnectSignal<WirelessManager>(wm, SIGNAL(linkQualityForConnectedNetwork(double)), SIGNAL(wmLinkQualityForConnectedNetwork(double)));
            ndbConnectSignal<WirelessManager>(wm, SIGNAL(macAddressAvailable(QString)), SIGNAL(wmMacAddressAvailable(QString)));
        } else {
            NDB_LOG("could not get shared WirelessManager pointer");
        }
    } else {
        NDB_LOG("could not dlsym WirelessManager::sharedInstance");
    }
}

QString NickelDBus::version() {
    return QStringLiteral(NDB_VERSION);
}

bool NickelDBus::ndbInUSBMS() {
    return this->PlugManager__gadgetMode(PlugManager__sharedInstance());
}

QString NickelDBus::nickelClassDetails(QString const& static_metaobject_symbol) {
    NDB_ASSERT(QStringLiteral("ERROR: In USB session"), !ndbInUSBMS(), "not calling method nickelClassDetails: in usbms session");
    typedef QMetaObject NickelMetaObject;
    NDB_ASSERT(QStringLiteral("ERROR: not a valid staticMetaObject symbol"), static_metaobject_symbol.endsWith(QStringLiteral("staticMetaObjectE")), "not a valid staticMetaObject symbol");
    QByteArray sym = static_metaobject_symbol.toLatin1();
    NickelMetaObject *nmo;
    reinterpret_cast<void*&>(nmo) = dlsym(this->libnickel, sym.constData());
    NDB_ASSERT(QStringLiteral("ERROR: DLSYM"), nmo, "could not dlsym staticMetaObject function for symbol %s", sym.constData());
    QString str = QStringLiteral("");
    str.append(QString("Showing meta information for Nickel class %1 : \n").arg(nmo->className()));
    str.append("Properties : \n");
    for (int i = nmo->propertyOffset(); i < nmo->propertyCount(); ++i) {
        QMetaProperty prop = nmo->property(i);
        str.append(QString("\t%1 %2 :: readable: %3 :: writeable: %4\n").arg(prop.typeName()).arg(prop.name()).arg(prop.isReadable()).arg(prop.isWritable())); 
    }
    str.append("Methods : \n");
    for (int i = nmo->methodOffset(); i < nmo->methodCount(); ++i) {
        QMetaMethod method = nmo->method(i);
        const char *method_type;
        switch (method.methodType()) {
            case QMetaMethod::Signal:
                method_type = "SIGNAL";
                break;
            case QMetaMethod::Slot:
                method_type = "SLOT";
                break;
            case QMetaMethod::Method:
                method_type = "METHOD";
                break;
            case QMetaMethod::Constructor:
                method_type = "CONSTRUCTOR";
                break;
            default:
                method_type = "UNKOWN";
        }
        str.append(QString("\t%1 :: %2 %3\n").arg(method_type).arg(method.typeName()).arg(method.methodSignature().constData()));
    }
    return str;
}
bool NickelDBus::testAssert(bool test) {
    NDB_ASSERT(false, test, "The test value was '%s'", (test ? "true" : "false"));
    return true;
}

bool NickelDBus::signalConnected(QString const &signal_name) {
    return connectedSignals.contains(signal_name);
}

int NickelDBus::showToast(int toast_duration, QString const &msg_main, QString const &msg_sub) {
    NDB_ASSERT(ndb_err_usb, !ndbInUSBMS(), "not calling method showToast: in usbms session");
    // The following code has been adapted from NickelMenu
    NDB_ASSERT(ndb_err_inval_param, toast_duration > 0 && toast_duration <= 5000, "toast duration must be between 0 and 5000 miliseconds");
    MainWindowController *(*MainWindowController_sharedInstance)();
    void (*MainWindowController_toast)(MainWindowController*, QString const&, QString const&, int);
    //libnickel 4.6 * _ZN20MainWindowController14sharedInstanceEv
    reinterpret_cast<void*&>(MainWindowController_sharedInstance) = dlsym(libnickel, "_ZN20MainWindowController14sharedInstanceEv");
    NDB_ASSERT(ndb_err_dlsym, MainWindowController_sharedInstance, "unsupported firmware: could not find MainWindowController::sharedInstance()");
    //libnickel 4.6 * _ZN20MainWindowController5toastERK7QStringS2_i
    reinterpret_cast<void*&>(MainWindowController_toast) = dlsym(libnickel, "_ZN20MainWindowController5toastERK7QStringS2_i");
    NDB_ASSERT(ndb_err_dlsym, MainWindowController_toast, "unsupported firmware: could not find MainWindowController::toast(QString const&, QString const&, int)");
    MainWindowController *mwc = MainWindowController_sharedInstance();
    NDB_ASSERT(ndb_err_call, mwc, "could not get MainWindowController instance");
    MainWindowController_toast(mwc, msg_main, msg_sub, toast_duration);
    return ndb_err_ok;
}
int NickelDBus::goHome() {
    NDB_ASSERT(ndb_err_usb, !ndbInUSBMS(), "not calling method goHome: in usbms session");
    return ndbNickelMisc("home");
}

int NickelDBus::pfmRescanBooks() {
    NDB_ASSERT(ndb_err_usb, !ndbInUSBMS(), "not calling method pfmRescanBooks: in usbms session");
    return ndbNickelMisc("rescan_books");
}

int NickelDBus::pfmRescanBooksFull() {
    NDB_ASSERT(ndb_err_usb, !ndbInUSBMS(), "not calling method pfmRescanBooksFull: in usbms session");
    return ndbNickelMisc("rescan_books_full");
}

int NickelDBus::ndbNickelMisc(const char *action) {
    char *err = NULL;
    nm_action_result_t *res = nm_action_nickel_misc(action, &err);
    if (!res) {
        NDB_LOG("nm_action_nickel_misc failed with error: %s", err);
        free(err);
        return ndb_err_call;
    }
    nm_action_result_free(res);
    return ndb_err_ok;
}

bool NickelDBus::ndbActionStrValid(QString const& actStr) {
    return (!actStr.compare("enable") || !actStr.compare("disable") || !actStr.compare("toggle"));
}

int NickelDBus::wfmConnectWireless() {
    NDB_ASSERT(ndb_err_usb, !ndbInUSBMS(), "not calling method wfmConnectWireless: in usbms session");
    return ndbWireless("autoconnect");
}
int NickelDBus::wfmConnectWirelessSilently() {
    NDB_ASSERT(ndb_err_usb, !ndbInUSBMS(), "not calling method wfmConnectWirelessSilently: in usbms session");
    return ndbWireless("autoconnect_silent");
}
int NickelDBus::wfmSetAirplaneMode(QString const& action) {
    NDB_ASSERT(ndb_err_usb, !ndbInUSBMS(), "not calling method wfmSetAirplaneMode: in usbms session");
    NDB_ASSERT(ndb_err_inval_param, ndbActionStrValid(action), "invalid action name");
    QByteArray actBytes = action.toUtf8();
    return ndbWireless(actBytes.constData());
}

int NickelDBus::ndbWireless(const char *act) {
    char *err;
    nm_action_result_t *res = nm_action_nickel_wifi(act, &err);
    if (!res) {
        NDB_LOG("ndbWireless failed with error: %s", err);
        free(err);
        return ndb_err_call;
    }
    nm_action_result_free(res);
    return ndb_err_ok;
}

int NickelDBus::bwmOpenBrowser(bool modal, QString const& url, QString const& css) {
    NDB_ASSERT(ndb_err_usb, !ndbInUSBMS(), "not calling method bwmOpenBrowser: in usbms session");
    QString qarg = QStringLiteral("");
    if (modal || !url.isEmpty() || !css.isEmpty()) {
        if (modal) {
            qarg.append("modal");
            if (!url.isEmpty() || !css.isEmpty()) {
                qarg.append(":");
            }
        }
        if (!url.isEmpty()) {
            qarg.append(QString("%1 ").arg(url));
        }
        if (!css.isEmpty()) {
            qarg.append(css);
        }
    }
    QByteArray qb_arg;
    if (!qarg.isEmpty()) {
        qb_arg = qarg.toUtf8();
    } else {
        qb_arg = QByteArray();
    }
    char *err = NULL;
    nm_action_result_t *res = nm_action_nickel_browser((qb_arg.isEmpty() ? NULL : qb_arg.constData()), &err);
    if (!res) {
        NDB_LOG("bwmOpenBrowser failed with error: %s", err);
        free(err);
        return ndb_err_call;
    }
    nm_action_result_free(res);
    return ndb_err_ok;
}

int NickelDBus::nsInvert(QString const& action) {
    NDB_ASSERT(ndb_err_usb, !ndbInUSBMS(), "not calling method nsInvert: in usbms session");
    return ndbSettings(action, "invert");
}
int NickelDBus::nsLockscreen(QString const& action) {
    NDB_ASSERT(ndb_err_usb, !ndbInUSBMS(), "not calling method nsLockscreen: in usbms session");
    return ndbSettings(action, "lockscreen");
}
int NickelDBus::nsScreenshots(QString const& action) {
    NDB_ASSERT(ndb_err_usb, !ndbInUSBMS(), "not calling method nsScreenshots: in usbms session");
    return ndbSettings(action, "screenshots");
}
int NickelDBus::nsForceWifi(QString const& action) {
    NDB_ASSERT(ndb_err_usb, !ndbInUSBMS(), "not calling method nsForceWifi: in usbms session");
    return ndbSettings(action, "force_wifi");
}
int NickelDBus::ndbSettings(QString const& action, const char* setting) {
    NDB_ASSERT(ndb_err_inval_param, ndbActionStrValid(action), "invalid action name");
    QByteArray qarg = QString("%1:%2").arg(action).arg(setting).toUtf8();
    char *err;
    nm_action_result_t *res = nm_action_nickel_setting(qarg.constData(), &err);
    if (!res) {
        NDB_LOG("ndbSettings failed with error: %s", err);
        free(err);
        return ndb_err_call;
    }
    nm_action_result_free(res);
    return ndb_err_ok;
}
