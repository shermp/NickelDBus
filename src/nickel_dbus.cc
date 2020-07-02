#include <dlfcn.h>
#include <QString>
#include <unistd.h>
#include "../NickelMenu/src/action.h"
#include "util.h"
#include "nickel_dbus.h"
#include "adapter/nickel_dbus_adapter.h"

typedef QObject PlugWorkflowManager;
typedef QObject WirelessManager;
typedef void MainWindowController;

NickelDBus::NickelDBus(QObject* parent) : QObject(parent) {
    new NickelDBusAdapter(this);
    this->dbusRegSucceeded = true;
    QDBusConnection conn = QDBusConnection::systemBus();
    if (!conn.registerObject("/nickeldbus", this)) {
        NDB_LOG("failed to register object on system bus");
        this->dbusRegSucceeded = false;
    }
    if (!conn.registerService("local.shermp.nickeldbus")) {
        NDB_LOG("failed to register service on the system bus");
        this->dbusRegSucceeded = false;
    }
    this->libnickel = nullptr;
    this->methodsInhibited = false;
}
NickelDBus::~NickelDBus() {
    QDBusConnection conn = QDBusConnection::systemBus();
    conn.unregisterService("local.shermp.nickeldbus");
    conn.unregisterObject("/nickeldbus");
}

void NickelDBus::connectSignals() {
    PlugWorkflowManager *(*PlugWorkflowManager_sharedInstance)();
    reinterpret_cast<void*&>(PlugWorkflowManager_sharedInstance) = dlsym(this->libnickel, "_ZN19PlugWorkflowManager14sharedInstanceEv");
    if (PlugWorkflowManager_sharedInstance) {
        PlugWorkflowManager *wf = PlugWorkflowManager_sharedInstance();
        if (wf) {
            NDB_LOG("connecting PlugWorkflowManager::aboutToConnect");
            if (QObject::connect(wf, SIGNAL(aboutToConnect()), this, SIGNAL(pfmAboutToConnect()))) {
                connectedSignals.insert("pfmAboutToConnect");
            } else {NDB_LOG("PlugWorkflowManager::aboutToConnect connection failed");}
            // This seems an appropriate signal to also use to inhibit calling actions while nickel is
            // exporting a USBMS session
            if (!QObject::connect(wf, SIGNAL(aboutToConnect()), this, SLOT(enableMethodInhibit()))) {
                NDB_LOG("PlugWorkflowManager::aboutToConnect connection to enableMethodInhibit failed");
            }
            NDB_LOG("connecting PlugWorkflowManager::doneProcessing");
            if (QObject::connect(wf, SIGNAL(doneProcessing()), this, SIGNAL(pfmDoneProcessing()))) {
                connectedSignals.insert("pfmDoneProccessing");
            } else {NDB_LOG("PlugWorkflowManager::doneProcessing connection failed");}
            // And it should be safe to allow calling actions after this signal
            if (!QObject::connect(wf, SIGNAL(doneProcessing()), this, SLOT(disableMethodInhibit()))) {
                NDB_LOG("PlugWorkflowManager::doneProcessing connection to disableMethodInhibit failed");
            }
        } else {NDB_LOG("could not get shared PlugWorkflowManager pointer");}
    } else {NDB_LOG("could not dlsym PlugWorkflowManager::sharedInstance");}

    WirelessManager *(*WirelesManager_sharedInstance)();
    reinterpret_cast<void*&>(WirelesManager_sharedInstance) = dlsym(this->libnickel, "_ZN15WirelessManager14sharedInstanceEv");
    if (WirelesManager_sharedInstance) {
        WirelessManager *wm = WirelesManager_sharedInstance();
        if (wm) {
            NDB_LOG("connecting WirelessManager::tryingToConnect()");
            if (QObject::connect(wm, SIGNAL(tryingToConnect()), this, SIGNAL(wmTryingToConnect()))) {
                connectedSignals.insert("wmTryingToConnect");
            } else {NDB_LOG("WirelessManager::tryingToConnect() connection failed");}
            NDB_LOG("connecting WirelessManager::networkConnected()");
            if (QObject::connect(wm, SIGNAL(networkConnected()), this, SIGNAL(wmNetworkConnected()))) {
                connectedSignals.insert("wmNetworkConnected");
            } else {NDB_LOG("WirelessManager::networkConnected() connection failed");}
            NDB_LOG("connecting WirelessManager::networkDisconnected()");
            if (QObject::connect(wm, SIGNAL(networkDisconnected()), this, SIGNAL(wmNetworkDisconnected()))) {
                connectedSignals.insert("wmNetworkDisconnected");
            } else {NDB_LOG("WirelessManager::networkDisconnected() connection failed");}
            NDB_LOG("connecting WirelessManager::networkForgotten()");
            if (QObject::connect(wm, SIGNAL(networkForgotten()), this, SIGNAL(wmNetworkForgotten()))) {
                connectedSignals.insert("wmNetworkForgotten");
            } else {NDB_LOG("WirelessManager::networkForgotten() connection failed");}
            NDB_LOG("connecting WirelessManager::networkFailedToConnect()");
            if (QObject::connect(wm, SIGNAL(networkFailedToConnect()), this, SIGNAL(wmNetworkFailedToConnect()))) {
                connectedSignals.insert("wmNetworkFailedToConnect");
            } else {NDB_LOG("WirelessManager::networkFailedToConnect() connection failed");}
            NDB_LOG("connecting WirelessManager::scanningStarted()");
            if (QObject::connect(wm, SIGNAL(scanningStarted()), this, SIGNAL(wmScanningStarted()))) {
                connectedSignals.insert("wmScanningStarted");
            } else {NDB_LOG("WirelessManager::scanningStarted() connection failed");}
            NDB_LOG("connecting WirelessManager::scanningFinished()");
            if (QObject::connect(wm, SIGNAL(scanningFinished()), this, SIGNAL(wmScanningFinished()))) {
                connectedSignals.insert("wmScanningFinished");
            } else {NDB_LOG("WirelessManager::scanningFinished() connection failed");}
            NDB_LOG("connecting WirelessManager::scanningAborted()");
            if (QObject::connect(wm, SIGNAL(scanningAborted()), this, SIGNAL(wmScanningAborted()))) {
                connectedSignals.insert("wmScanningAborted");
            } else {NDB_LOG("WirelessManager::scanningAborted() connection failed");}
            NDB_LOG("connecting WirelessManager::wifiEnabled()");
            if (QObject::connect(wm, SIGNAL(wifiEnabled(bool)), this, SIGNAL(wmWifiEnabled(bool)))) {
                connectedSignals.insert("wmWifiEnabled");
            } else {NDB_LOG("WirelessManager::wifiEnabled() connection failed");}
            NDB_LOG("connecting WirelessManager::linkQualityForConnectedNetwork()");
            if (QObject::connect(wm, SIGNAL(linkQualityForConnectedNetwork(double)), this, SIGNAL(wmLinkQualityForConnectedNetwork(double)))) {
                connectedSignals.insert("wmLinkQualityForConnectedNetwork");
            } else {NDB_LOG("WirelessManager::linkQualityForConnectedNetwork() connection failed");}
            NDB_LOG("connecting WirelessManager::macAddressAvailable()");
            if (QObject::connect(wm, SIGNAL(macAddressAvailable(QString)), this, SIGNAL(wmMacAddressAvailable(QString)))) {
                connectedSignals.insert("wmMacAddressAvailable");
            } else {NDB_LOG("WirelessManager::macAddressAvailable() connection failed");}

        }
    }
}

QString NickelDBus::version() {
    return QString(NDB_VERSION);
}

void NickelDBus::enableMethodInhibit() {
    this->methodsInhibited = true;
}

void NickelDBus::disableMethodInhibit() {
    this->methodsInhibited = false;
}

QString NickelDBus::nickelClassDetails(QString const& static_metaobject_symbol) {
    NDB_ASSERT(QString("ERROR: In USB session"), !this->methodsInhibited, "not calling method nickelClassDetails: in usbms session");
    typedef QMetaObject NickelMetaObject;
    NDB_ASSERT(QString("ERROR: not a valid staticMetaObject symbol"), static_metaobject_symbol.endsWith(QString("staticMetaObjectE")), "not a valid staticMetaObject symbol");
    QByteArray sym = static_metaobject_symbol.toLatin1();
    NickelMetaObject *nmo;
    reinterpret_cast<void*&>(nmo) = dlsym(this->libnickel, sym.constData());
    NDB_ASSERT(QString("ERROR: DLSYM"), nmo, "could not dlsym staticMetaObject function for symbol %s", sym.constData());
    QString str = QString("");
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
    NDB_ASSERT(ndb_err_usb, !this->methodsInhibited, "not calling method showToast: in usbms session");
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
    NDB_ASSERT(ndb_err_usb, !this->methodsInhibited, "not calling method goHome: in usbms session");
    return ndbNickelMisc("home");
}

int NickelDBus::pfmRescanBooks() {
    NDB_ASSERT(ndb_err_usb, !this->methodsInhibited, "not calling method pfmRescanBooks: in usbms session");
    return ndbNickelMisc("rescan_books");
}

int NickelDBus::pfmRescanBooksFull() {
    NDB_ASSERT(ndb_err_usb, !this->methodsInhibited, "not calling method pfmRescanBooksFull: in usbms session");
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

NickelDBus::nm_action NickelDBus::parseActionStr(QString const& actStr) {
    NickelDBus::nm_action act;
    if (!actStr.compare("enable", Qt::CaseInsensitive)) {act = NM_ACT_ENABLE;} 
    else if (!actStr.compare("disable", Qt::CaseInsensitive)) {act = NM_ACT_DISABLE;}
    else if (!actStr.compare("toggle", Qt::CaseInsensitive)) {act = NM_ACT_TOGGLE;}
    else {act = NM_ACT_ERR;}
    return act;
}

int NickelDBus::wfmConnectWireless() {
    NDB_ASSERT(ndb_err_usb, !this->methodsInhibited, "not calling method wfmConnectWireless: in usbms session");
    return ndbWireless(NM_ACT_AUTO);
}
int NickelDBus::wfmConnectWirelessSilently() {
    NDB_ASSERT(ndb_err_usb, !this->methodsInhibited, "not calling method wfmConnectWirelessSilently: in usbms session");
    return ndbWireless(NM_ACT_AUTO_SILENT);
}
int NickelDBus::wfmSetAirplaneMode(QString const& action) {
    NDB_ASSERT(ndb_err_usb, !this->methodsInhibited, "not calling method wfmSetAirplaneMode: in usbms session");
    NickelDBus::nm_action act = parseActionStr(action);
    NDB_ASSERT(ndb_err_inval_param, act != NM_ACT_ERR, "invalid action name");
    //enum wireless_conn_option opt = enabled ? ENABLE : DISABLE;
    return ndbWireless(act);
}

int NickelDBus::ndbWireless(enum nm_action act) {
    const char *arg;
    switch (act) {
    case NM_ACT_AUTO: 
        arg = "autoconnect"; 
        break;
    case NM_ACT_AUTO_SILENT:
        arg = "autoconnect_silent";
        break;
    case NM_ACT_ENABLE:
        arg = "enable";
        break;
    case NM_ACT_DISABLE:
        arg = "disable";
        break;
    case NM_ACT_TOGGLE:
        arg = "toggle";
        break;
    default:
        return ndb_err_inval_param; // keep GCC happy
    }
    char *err;
    nm_action_result_t *res = nm_action_nickel_wifi(arg, &err);
    if (!res) {
        NDB_LOG("ndbWireless failed with error: %s", err);
        free(err);
        return ndb_err_call;
    }
    nm_action_result_free(res);
    return ndb_err_ok;
}

int NickelDBus::bwmOpenBrowser(bool modal, QString const& url, QString const& css) {
    NDB_ASSERT(ndb_err_usb, !this->methodsInhibited, "not calling method bwmOpenBrowser: in usbms session");
    QString qarg = QString("");
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
    NDB_ASSERT(ndb_err_usb, !this->methodsInhibited, "not calling method nsInvert: in usbms session");
    return ndbSettings(action, QString("invert"));
}
int NickelDBus::nsLockscreen(QString const& action) {
    NDB_ASSERT(ndb_err_usb, !this->methodsInhibited, "not calling method nsLockscreen: in usbms session");
    return ndbSettings(action, QString("lockscreen"));
}
int NickelDBus::nsScreenshots(QString const& action) {
    NDB_ASSERT(ndb_err_usb, !this->methodsInhibited, "not calling method nsScreenshots: in usbms session");
    return ndbSettings(action, QString("screenshots"));
}
int NickelDBus::nsForceWifi(QString const& action) {
    NDB_ASSERT(ndb_err_usb, !this->methodsInhibited, "not calling method nsForceWifi: in usbms session");
    return ndbSettings(action, QString("force_wifi"));
}
int NickelDBus::ndbSettings(QString const& action, QString const& setting) {
    NickelDBus::nm_action act = parseActionStr(action);
    NDB_ASSERT(ndb_err_inval_param, act != NM_ACT_ERR, "invalid action name");
    QString nm_action;
    switch (act) {
    case NM_ACT_ENABLE:
        nm_action = QString("enable");
        break;
    case NM_ACT_DISABLE:
        nm_action = QString("disable");
        break;
    case NM_ACT_TOGGLE:
        nm_action = QString("toggle");
        break;
    default:
        return ndb_err_inval_param; // keep GCC happy
    }
    QByteArray qarg = QString("%1:%2").arg(nm_action, setting).toLatin1();
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
