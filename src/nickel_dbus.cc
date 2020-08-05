#include <dlfcn.h>
#include <QString>
#include <unistd.h>
#include <string.h>
#include <NickelHook.h>
#include "../NickelMenu/src/action.h"
#include "../NickelMenu/src/util.h"
#include "util.h"
#include "nickel_dbus.h"
#include "adapter/nickel_dbus_adapter.h"

NickelDBus::NickelDBus(QObject* parent) : QObject(parent), QDBusContext() {
    new NickelDBusAdapter(this);
    initSucceeded = true;
    nh_log("NickelDBus: registering object %s", NDB_DBUS_OBJECT_PATH);
    if (!conn.registerObject(NDB_DBUS_OBJECT_PATH, this)) {
        nh_log("NickelDBus: failed to register object on system bus");
        initSucceeded = false;
        return;
    }
    nh_log("NickelDBus: registering interface %s", NDB_DBUS_IFACE_NAME);
    if (!conn.registerService(NDB_DBUS_IFACE_NAME)) {
        nh_log("NickelDBus: failed to register service on the system bus");
        initSucceeded = false;
        return;
    }
    libnickel = dlopen("libnickel.so.1.0.0", RTLD_LAZY|RTLD_NODELETE);
    if (!libnickel) {
        nh_log("NickelDBus: could not dlopen libnickel");
        initSucceeded = false;
        return;
    }
    // The following symbols are required. If they can't be resolved, bail out
    ndbResolveSymbol("_ZN11PlugManager14sharedInstanceEv", nh_symoutptr(nSym.PlugManager__sharedInstance));
    ndbResolveSymbol("_ZNK11PlugManager10gadgetModeEv", nh_symoutptr(nSym.PlugManager__gadgetMode));
    if (!nSym.PlugManager__sharedInstance || !nSym.PlugManager__gadgetMode) {
        initSucceeded = false;
        return;
    }
    // Resolve the rest of the Nickel symbols up-front
    // PlugWorkFlowManager
    ndbResolveSymbol("_ZN19PlugWorkflowManager14sharedInstanceEv", nh_symoutptr(nSym.PlugWorkflowManager_sharedInstance));
    // WirelessManager
    ndbResolveSymbol("_ZN15WirelessManager14sharedInstanceEv", nh_symoutptr(nSym.WirelesManager_sharedInstance));
    // Confirmation Dialog
    ndbResolveSymbol("_ZN25ConfirmationDialogFactory21getConfirmationDialogEP7QWidget", nh_symoutptr(nSym.ConfirmationDialogFactory_getConfirmationDialog));
    ndbResolveSymbol("_ZN18ConfirmationDialog8setTitleERK7QString", nh_symoutptr(nSym.ConfirmationDialog__setTitle));
    ndbResolveSymbol("_ZN18ConfirmationDialog7setTextERK7QString", nh_symoutptr(nSym.ConfirmationDialog__setText));
    ndbResolveSymbol("_ZN18ConfirmationDialog19setAcceptButtonTextERK7QString", nh_symoutptr(nSym.ConfirmationDialog__setAcceptButtonText));
    ndbResolveSymbol("_ZN18ConfirmationDialog19setRejectButtonTextERK7QString", nh_symoutptr(nSym.ConfirmationDialog__setRejectButtonText));
    // Toast
    ndbResolveSymbol("_ZN20MainWindowController14sharedInstanceEv", nh_symoutptr(nSym.MainWindowController_sharedInstance));
    ndbResolveSymbol("_ZN20MainWindowController5toastERK7QStringS2_i", nh_symoutptr(nSym.MainWindowController_toast));
}

NickelDBus::~NickelDBus() {
    conn.unregisterService(NDB_DBUS_IFACE_NAME);
    conn.unregisterObject(NDB_DBUS_OBJECT_PATH);
}

void NickelDBus::ndbResolveSymbol(const char *name, void **fn) {
    if (!(*fn = dlsym(libnickel, name))) {
        nh_log("info... could not load %s", name);
    }
}

template <typename T>
void NickelDBus::ndbConnectSignal(T *srcObj, const char *srcSignal, const char *dest) {
    const char *dest_start = dest + 1;
    const char *dest_end = strchr(dest_start, '(');
    nh_log("connecting %s to %s", srcSignal, dest);
    if (QObject::connect(srcObj, srcSignal, this, dest)) {
        connectedSignals.insert(QString::fromLatin1(dest_start, dest_end - dest_start));
    } else {
        nh_log("failed to connect %s to %s", srcSignal, dest);
    }
}

void NickelDBus::connectSignals() {
    if (nSym.PlugWorkflowManager_sharedInstance) {
        PlugWorkflowManager *wf = nSym.PlugWorkflowManager_sharedInstance();
        if (wf) {
            ndbConnectSignal<PlugWorkflowManager>(wf, SIGNAL(aboutToConnect()), SIGNAL(pfmAboutToConnect()));
            ndbConnectSignal<PlugWorkflowManager>(wf, SIGNAL(doneProcessing()), SIGNAL(pfmDoneProcessing()));
        } else {
            nh_log("could not get shared PlugWorkflowManager pointer");
        }
    }
    if (nSym.WirelesManager_sharedInstance) {
        WirelessManager *wm = nSym.WirelesManager_sharedInstance();
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
            nh_log("could not get shared WirelessManager pointer");
        }
    }
}

QString NickelDBus::ndbVersion() {
    return QStringLiteral(NH_VERSION);
}

bool NickelDBus::ndbInUSBMS() {
    return nSym.PlugManager__gadgetMode(nSym.PlugManager__sharedInstance());
}

QString NickelDBus::getNickelMetaObjectDetails(const QMetaObject* nmo) {
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

QString NickelDBus::miscNickelClassDetails(QString const& static_metaobject_symbol) {
    NDB_DBUS_USB_ASSERT(QString(""));
    typedef QMetaObject NickelMetaObject;
    NDB_DBUS_ASSERT(QString(""),QDBusError::InvalidArgs, static_metaobject_symbol.endsWith(QStringLiteral("staticMetaObjectE")), "not a valid staticMetaObject symbol");
    QByteArray sym = static_metaobject_symbol.toLatin1();
    NickelMetaObject *nmo;
    reinterpret_cast<void*&>(nmo) = dlsym(libnickel, sym.constData());
    NDB_DBUS_ASSERT(QString(""), QDBusError::InternalError, nmo, "could not dlsym staticMetaObject function for symbol %s", sym.constData());
    return getNickelMetaObjectDetails((const NickelMetaObject*)nmo);
}

void NickelDBus::allowDialog() {
    allowDlg = true;
}

void NickelDBus::dlgConfirmation(QString const& title, QString const& body, QString const& acceptText, QString const& rejectText) {
    NDB_DBUS_ASSERT((void) 0, QDBusError::AccessDenied, allowDlg, "dialog already showing");
    allowDlg = false;
    NDB_DBUS_USB_ASSERT((void) 0);
    NDB_DBUS_SYM_ASSERT((void) 0, nSym.ConfirmationDialogFactory_getConfirmationDialog && nSym.ConfirmationDialog__setTitle &&
        nSym.ConfirmationDialog__setText && nSym.ConfirmationDialog__setAcceptButtonText && nSym.ConfirmationDialog__setRejectButtonText);
    ConfirmationDialog *dlg = nSym.ConfirmationDialogFactory_getConfirmationDialog(nullptr);
    NDB_DBUS_ASSERT((void) 0, QDBusError::InternalError, dlg, "error getting confirmation dialog");
    
    nSym.ConfirmationDialog__setTitle(dlg, title);
    nSym.ConfirmationDialog__setText(dlg, body);

    if (!acceptText.isEmpty()) { nSym.ConfirmationDialog__setAcceptButtonText(dlg, acceptText); }
    if (!rejectText.isEmpty()) { nSym.ConfirmationDialog__setRejectButtonText(dlg, rejectText); }

    dlg->setModal(true);
    QObject::connect(dlg, &QDialog::finished, this, &NickelDBus::dlgConfirmResult);
    QObject::connect(dlg, &QDialog::finished, this, &NickelDBus::allowDialog);
    QObject::connect(dlg, &QDialog::finished, dlg, &QDialog::deleteLater);
    dlg->open();
}

void NickelDBus::dlgConfirmNoBtn(QString const& title, QString const& body) {
    return dlgConfirmation(title, body, QString(""), QString(""));
}

void NickelDBus::dlgConfirmAccept(QString const& title, QString const& body, QString const& acceptText) {
    return dlgConfirmation(title, body, acceptText, QString(""));
}

void NickelDBus::dlgConfirmReject(QString const& title, QString const& body, QString const& rejectText) {
    return dlgConfirmation(title, body, QString(""), rejectText);
}

void NickelDBus::dlgConfirmAcceptReject(QString const& title, QString const& body, QString const& acceptText, QString const& rejectText) {
    return dlgConfirmation(title, body, acceptText, rejectText);
}

bool NickelDBus::miscSignalConnected(QString const &signal_name) {
    return connectedSignals.contains(signal_name);
}

void NickelDBus::mwcToast(int toast_duration, QString const &msg_main, QString const &msg_sub) {
    NDB_DBUS_USB_ASSERT((void) 0);
    // The following code has been adapted from NickelMenu
    NDB_DBUS_ASSERT((void) 0, QDBusError::InvalidArgs, toast_duration > 0 && toast_duration <= 5000, "toast duration must be between 0 and 5000 miliseconds");
    NDB_DBUS_SYM_ASSERT((void) 0, nSym.MainWindowController_sharedInstance && nSym.MainWindowController_toast);
    MainWindowController *mwc = nSym.MainWindowController_sharedInstance();
    NDB_DBUS_ASSERT((void) 0, QDBusError::InternalError, mwc, "could not get MainWindowController instance");
    nSym.MainWindowController_toast(mwc, msg_main, msg_sub, toast_duration);
}

void NickelDBus::mwcHome() {
    NDB_DBUS_USB_ASSERT((void) 0);
    return ndbNickelMisc("home");
}

void NickelDBus::pfmRescanBooks() {
    NDB_DBUS_USB_ASSERT((void) 0);
    return ndbNickelMisc("rescan_books");
}

void NickelDBus::pfmRescanBooksFull() {
    NDB_DBUS_USB_ASSERT((void) 0);
    return ndbNickelMisc("rescan_books_full");
}

void NickelDBus::ndbNickelMisc(const char *action) {
    nm_action_result_t *res = nm_action_nickel_misc(action);
    if (!res) {
        nh_log("nm_action_nickel_misc failed with error: %s", nm_err_peek());
        sendErrorReply(QDBusError::InternalError, QString("nm_action_nickel_misc failed with error: %1").arg(nm_err()));
        return;
    }
    nm_action_result_free(res);
}

bool NickelDBus::ndbActionStrValid(QString const& actStr) {
    return (!actStr.compare("enable") || !actStr.compare("disable") || !actStr.compare("toggle"));
}

void NickelDBus::wfmConnectWireless() {
    NDB_DBUS_USB_ASSERT((void) 0);
    return ndbWireless("autoconnect");
}

void NickelDBus::wfmConnectWirelessSilently() {
    NDB_DBUS_USB_ASSERT((void) 0);
    return ndbWireless("autoconnect_silent");
}

void NickelDBus::wfmSetAirplaneMode(QString const& action) {
    NDB_DBUS_USB_ASSERT((void) 0);
    NDB_DBUS_ASSERT((void) 0, QDBusError::InvalidArgs, ndbActionStrValid(action), "invalid action name");
    QByteArray actBytes = action.toUtf8();
    return ndbWireless(actBytes.constData());
}

void NickelDBus::ndbWireless(const char *act) {
    nm_action_result_t *res = nm_action_nickel_wifi(act);
    if (!res) {
        nh_log("ndbWireless failed with error: %s", nm_err_peek());
        sendErrorReply(QDBusError::InternalError, QString("ndbWireless failed with error: %1").arg(nm_err()));
        return;
    }
    nm_action_result_free(res);
}

void NickelDBus::bwmOpenBrowser(bool modal, QString const& url, QString const& css) {
    NDB_DBUS_USB_ASSERT((void) 0);
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
    nm_action_result_t *res = nm_action_nickel_browser((qb_arg.isEmpty() ? NULL : qb_arg.constData()));
    if (!res) {
        nh_log("bwmOpenBrowser failed with error: %s", nm_err_peek());
        sendErrorReply(QDBusError::InternalError, QString("bwmOpenBrowser failed with error: %1").arg(nm_err()));
        return;
    }
    nm_action_result_free(res);
}

void NickelDBus::nsInvert(QString const& action) {
    NDB_DBUS_USB_ASSERT((void) 0);
    return ndbSettings(action, "invert");
}

void NickelDBus::nsLockscreen(QString const& action) {
    NDB_DBUS_USB_ASSERT((void) 0);
    return ndbSettings(action, "lockscreen");
}

void NickelDBus::nsScreenshots(QString const& action) {
    NDB_DBUS_USB_ASSERT((void) 0);
    return ndbSettings(action, "screenshots");
}

void NickelDBus::nsForceWifi(QString const& action) {
    NDB_DBUS_USB_ASSERT((void) 0);
    return ndbSettings(action, "force_wifi");
}

void NickelDBus::ndbSettings(QString const& action, const char* setting) {
    NDB_DBUS_ASSERT((void) 0, QDBusError::InvalidArgs, ndbActionStrValid(action), "invalid action name");
    QByteArray qarg = QString("%1:%2").arg(action).arg(setting).toUtf8();
    nm_action_result_t *res = nm_action_nickel_setting(qarg.constData());
    if (!res) {
        nh_log("ndbSettings failed with error: %s", nm_err());
        sendErrorReply(QDBusError::InternalError, QString("ndbSettings failed with error: %1").arg(nm_err()));
        return;
    }
    nm_action_result_free(res);
}

void NickelDBus::pwrShutdown() {
    NDB_DBUS_USB_ASSERT((void) 0);
    return pwrAction("shutdown");
}

void NickelDBus::pwrReboot() {
    NDB_DBUS_USB_ASSERT((void) 0);
    return pwrAction("reboot");
}

void NickelDBus::pwrAction(const char *action) {
    nm_action_result_t *res = nm_action_power(action);
    if (!res) {
        nh_log("pwrAction failed with error: %s", nm_err_peek());
        sendErrorReply(QDBusError::InternalError, QString("pwrAction failed with error: %1").arg(nm_err()));
        return;
    }
    nm_action_result_free(res);
}