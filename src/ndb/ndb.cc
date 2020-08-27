#include <dlfcn.h>
#include <QApplication>
#include <QString>
#include <QWidget>
#include <unistd.h>
#include <string.h>
#include <NickelHook.h>
#include "../../NickelMenu/src/action.h"
#include "../../NickelMenu/src/util.h"
#include "util.h"
#include "ndb.h"
#include "../interface/ndb_adapter.h"

/*!
 * \class NDB
 * \brief The NDB class registers a service on d-bus of Kobo e-readers.
 * 
 * NDB provides a bridge between Kobo's proprietary software, libnickel, 
 * and other software running on Kobo e-readers. It registers itself as a 
 * service on d-bus, and provides methods and signals to monitor and interact 
 * with nickel.
 * 
 */

/*!
 * \internal
 * \brief Construct a new Nickel D-Bus object
 * 
 * \a parent QObject
 */
NDB::NDB(QObject* parent) : QObject(parent), QDBusContext() {
    new NDBAdapter(this);
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
    if (!nSym.PlugManager__gadgetMode) {
        // Older firmware versions use a slightly different mangled symbol
        ndbResolveSymbol("_ZN11PlugManager10gadgetModeEv", nh_symoutptr(nSym.PlugManager__gadgetMode));
    }
    if (!nSym.PlugManager__sharedInstance || !nSym.PlugManager__gadgetMode) {
        initSucceeded = false;
        return;
    }
    // Setup view change timer
    viewTimer = new QTimer(this);
    if (!viewTimer) {
        nh_log("failed to create viewTimer");
        initSucceeded = false;
        return;
    }
    viewTimer->setSingleShot(true);
    QObject::connect(viewTimer, &QTimer::timeout, this, &NDB::handleQSWTimer);

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
    // Get N3Dialog content
    ndbResolveSymbol("_ZN8N3Dialog7contentEv", nh_symoutptr(nSym.N3Dialog__content));
}

/*!
 * \internal
 * \brief Destroy the NDB::NDB object
 */
NDB::~NDB() {
    delete viewTimer;
    conn.unregisterService(NDB_DBUS_IFACE_NAME);
    conn.unregisterObject(NDB_DBUS_OBJECT_PATH);
}

void NDB::ndbResolveSymbol(const char *name, void **fn) {
    if (!(*fn = dlsym(libnickel, name))) {
        nh_log("info... could not load %s", name);
    }
}

template <typename T>
void NDB::ndbConnectSignal(T *srcObj, const char *srcSignal, const char *dest) {
    const char *dest_start = dest + 1;
    const char *dest_end = strchr(dest_start, '(');
    nh_log("connecting %s to %s", srcSignal, dest);
    if (QObject::connect(srcObj, srcSignal, this, dest)) {
        connectedSignals.insert(QString::fromLatin1(dest_start, dest_end - dest_start));
    } else {
        nh_log("failed to connect %s to %s", srcSignal, dest);
    }
}

/*! 
 * \internal
 * \brief Connects available Nickel signals to d-bus
 * 
 * Failures to connect a signal will stop execution, the failure will be logged
 * to syslog. 
 */
void NDB::connectSignals() {
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

/*!
 * \brief Get the version of NickelDBus
 */
QString NDB::ndbVersion() {
    return QStringLiteral(NH_VERSION);
}

void NDB::handleStackedWidgetDestroyed() {
    stackedWidget = nullptr;
}

void NDB::handleQSWCurrentChanged(int index) {
    if (index >= 0) {
        // I'd rather emit the ndbViewChanged signal here, but it's
        // not reliable, so it seems it needs to wait until the signal
        // handler completes. Hence the timer.
        // This does give us a chance to filter out duplicate change
        // signals, as some firmware versions appear to do.
        if (!viewTimer->isActive()) {
            viewTimer->start(10);
        }
    }
}

void NDB::handleQSWTimer() {
    emit ndbViewChanged(ndbCurrentView());
}

/*!
 * \brief Get the class name of the current view.
 * 
 * Some class name examples are \c HomePageView \c ReadingView
 * among others.
 */
QString NDB::ndbCurrentView() {
    // The ReadingView widget can be found in the same QStackedWidget
    // as the HomePageView widget. We can search for it using the
    // appropriate QApplication static methods
    if (!stackedWidget) {
        QWidgetList wl = QApplication::allWidgets();
        for (int i = 0; i < wl.size(); ++i) {
            if (!QString(wl[i]->metaObject()->className()).compare("QStackedWidget")) {
                QStackedWidget *sw = static_cast<QStackedWidget*>(wl[i]);
                for (int j = 0; j < sw->count(); ++j) {
                    if (QWidget *w = sw->widget(j)) {
                        if (!QString(w->objectName()).compare("HomePageView")) {
                            stackedWidget = sw;
                            QObject::connect(stackedWidget, &QStackedWidget::currentChanged, this, &NDB::handleQSWCurrentChanged);
                            // Just in case Nickel ever decides to destroy the stacked widget, we'll connect its destroyed()
                            // signal to make sure we aren't left with a dangling pointer.
                            QObject::connect(stackedWidget, &QObject::destroyed, this, &NDB::handleStackedWidgetDestroyed);
                            break;
                        }
                    }
                }
            }
        }
    }
    NDB_DBUS_ASSERT(QString(), QDBusError::InternalError, stackedWidget, "unable to retrieve HomePageView stacked widget");
    QWidget *w = stackedWidget->currentWidget();
    NDB_DBUS_ASSERT(QString(), QDBusError::InternalError, w, "QStackedWidget has no current widget");
    QString name = QString(w->objectName());
    if (!name.compare("N3Dialog")) {
        NDB_DBUS_SYM_ASSERT(name, nSym.N3Dialog__content);
        if (QWidget *c = nSym.N3Dialog__content(w)) {
            name = c->objectName();
        }
    }
    return name;
}

bool NDB::ndbInUSBMS() {
    return nSym.PlugManager__gadgetMode(nSym.PlugManager__sharedInstance());
}

QString NDB::getNickelMetaObjectDetails(const QMetaObject* nmo) {
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

/*!
 * \brief Print available details from nickel classes
 * 
 * This method attempts to dlsym then parse the staticMetaObject
 * property available from the mangeled \a staticMetaobjectSymbol
 * 
 * A formatted string of available signals and slots is returned.
 */
QString NDB::ndbNickelClassDetails(QString const& staticMetaobjectSymbol) {
    NDB_DBUS_USB_ASSERT(QString(""));
    typedef QMetaObject NickelMetaObject;
    NDB_DBUS_ASSERT(QString(""),QDBusError::InvalidArgs, staticMetaobjectSymbol.endsWith(QStringLiteral("staticMetaObjectE")), "not a valid staticMetaObject symbol");
    QByteArray sym = staticMetaobjectSymbol.toLatin1();
    NickelMetaObject *nmo;
    reinterpret_cast<void*&>(nmo) = dlsym(libnickel, sym.constData());
    NDB_DBUS_ASSERT(QString(""), QDBusError::InternalError, nmo, "could not dlsym staticMetaObject function for symbol %s", sym.constData());
    return getNickelMetaObjectDetails((const NickelMetaObject*)nmo);
}

/*!
 * \brief Check if a signal was successfully connected
 * 
 * Check if \a signalName is connected. \a signalName must be provided
 * without parentheses and parameters.
 * 
 * Returns \c 1 if exists, or \c 0 otherwise
 */
bool NDB::ndbSignalConnected(QString const &signalName) {
    return connectedSignals.contains(signalName);
}

/*!
 * \internal
 * \brief Print details gleaned from the QApplication instance
 */
QString NDB::ndbNickelWidgets() {
    QString str = QString("Active Modal: \n");
    QWidget *modal = QApplication::activeModalWidget();
    if (modal) {
        str.append(QString("%1\n").arg(modal->metaObject()->className()));
    }
    str.append("\nActive Window: \n");
    QWidget *window = QApplication::activeWindow();
    if (window) {
        str.append(QString("%1\n").arg(window->metaObject()->className()));
    }
    str.append("\nFocused widget: \n");
    QWidget *focus = QApplication::focusWidget();
    if (focus) {
        str.append(QString("%1\n").arg(focus->metaObject()->className()));
    }
    str.append("\nAll Widgets: \n");
    QWidgetList widgets = QApplication::allWidgets();
    for (int i = 0; i < widgets.size(); ++i) {
        str.append(QString("%1\n").arg(widgets[i]->metaObject()->className()));
    }
    str.append("\nReading View State: \n");
    QWidgetList visWidgets = QApplication::allWidgets();
    for (int i = 0; i < visWidgets.size(); ++i) {
        if (!QString(visWidgets[i]->metaObject()->className()).compare("ReadingView")) {
            if (!visWidgets[i]->isHidden()) {
                str.append("visible\n");
            }
            str.append("\nReadingView Hierachy: ");
            QWidget *w = visWidgets[i];
            while (w) {
                str.append(QString("%1 -> ").arg(w->metaObject()->className()));
                w = w->parentWidget();
            }
            str.append("\n");
        }
    }
    str.append("\nStacked Widgets: \n");
    for (int i = 0; i < visWidgets.size(); ++i) {
        if (!QString(visWidgets[i]->metaObject()->className()).compare("QStackedWidget")) {
            str.append("\nWidgets in Stack: \n");
            QStackedWidget *sw = static_cast<QStackedWidget *>(visWidgets[i]);
            for (int j = 0; j < sw->count(); ++j) {
                if (QWidget *w = sw->widget(j)) {
                    str.append(QString("%1\n").arg(w->metaObject()->className()));
                }
            }
        }
    }

    // foreach (QWidget *widget, QApplication::topLevelWidgets()) {
    //     if (!widget->isHidden())
    //         str.append("%1").arg(widget->metaObject()->className());
    // }
    return str;
}

/*!
 * \internal
 * \brief Internal slot to enable new dialogs to be created 
 */
void NDB::allowDialog() {
    allowDlg = true;
}

void NDB::dlgConfirmation(QString const& title, QString const& body, QString const& acceptText, QString const& rejectText) {
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
    QObject::connect(dlg, &QDialog::finished, this, &NDB::dlgConfirmResult);
    QObject::connect(dlg, &QDialog::finished, this, &NDB::allowDialog);
    QObject::connect(dlg, &QDialog::finished, dlg, &QDialog::deleteLater);
    dlg->open();
}

/*!
 * \brief Show a confirmation dialog with no buttons (except close)
 * 
 * Create a dialog box with \a title and \a body. This dialog only has a close
 * button.
 * 
 * When the dialog is closed, a \l dlgConfirmResult() signal is emitted.
 */
void NDB::dlgConfirmNoBtn(QString const& title, QString const& body) {
    return dlgConfirmation(title, body, QString(""), QString(""));
}

/*!
 * \brief Show a confirmation dialog with accept and close buttons
 *
 * Create a dialog box with \a title and \a body. This dialog has a close
 * button, and an accept button labeled with \a acceptText
 *
 * When the dialog is closed, or the accept button is pressed, a 
 * \l dlgConfirmResult() signal is emitted.
 */
void NDB::dlgConfirmAccept(QString const& title, QString const& body, QString const& acceptText) {
    return dlgConfirmation(title, body, acceptText, QString(""));
}

/*!
 * \brief Show a confirmation dialog with reject and close buttons
 *
 * Create a dialog box with \a title and \a body. This dialog has a close
 * button, and a reject button labeled with \a rejectText
 *
 * When the dialog is closed, or the reject button is pressed, a 
 * \l dlgConfirmResult() signal is emitted.
 */
void NDB::dlgConfirmReject(QString const& title, QString const& body, QString const& rejectText) {
    return dlgConfirmation(title, body, QString(""), rejectText);
}

/*!
 * \brief Show a confirmation dialog with reject and close buttons
 *
 * Create a dialog box with \a title and \a body. This dialog has a close
 * button, a reject button labeled with \a rejectText, and an accept
 * button labeled with \a acceptText.
 *
 * When the dialog is closed, either button is pressed, a 
 * \l dlgConfirmResult() signal is emitted.
 */
void NDB::dlgConfirmAcceptReject(QString const& title, QString const& body, QString const& acceptText, QString const& rejectText) {
    return dlgConfirmation(title, body, acceptText, rejectText);
}

/*!
 * \brief Show a small, temporary text box with a message 
 * 
 * Show a text box on screen for \a toastDuration duration (in milliseconds)
 * with \a msgMain as the body text, and an optional \a msgSub
 */
void NDB::mwcToast(int toastDuration, QString const &msgMain, QString const &msgSub) {
    NDB_DBUS_USB_ASSERT((void) 0);
    // The following code has been adapted from NickelMenu
    NDB_DBUS_ASSERT((void) 0, QDBusError::InvalidArgs, toastDuration > 0 && toastDuration <= 5000, "toast duration must be between 0 and 5000 miliseconds");
    NDB_DBUS_SYM_ASSERT((void) 0, nSym.MainWindowController_sharedInstance && nSym.MainWindowController_toast);
    MainWindowController *mwc = nSym.MainWindowController_sharedInstance();
    NDB_DBUS_ASSERT((void) 0, QDBusError::InternalError, mwc, "could not get MainWindowController instance");
    nSym.MainWindowController_toast(mwc, msgMain, msgSub, toastDuration);
}

/*!
 * \brief Navigate to the home screen
 */
void NDB::mwcHome() {
    NDB_DBUS_USB_ASSERT((void) 0);
    return ndbNickelMisc("home");
}

/*!
 * \brief Begin an abbreviated book rescan. Same as 'rescan_books' from NickelMenu
 */
void NDB::pfmRescanBooks() {
    NDB_DBUS_USB_ASSERT((void) 0);
    return ndbNickelMisc("rescan_books");
}

/*!
 * \brief Begins a full book rescan. Same as 'rescan_books_full' from NickelMenu
 */
void NDB::pfmRescanBooksFull() {
    NDB_DBUS_USB_ASSERT((void) 0);
    return ndbNickelMisc("rescan_books_full");
}

void NDB::ndbNickelMisc(const char *action) {
    nm_action_result_t *res = nm_action_nickel_misc(action);
    if (!res) {
        nh_log("nm_action_nickel_misc failed with error: %s", nm_err_peek());
        sendErrorReply(QDBusError::InternalError, QString("nm_action_nickel_misc failed with error: %1").arg(nm_err()));
        return;
    }
    nm_action_result_free(res);
}

bool NDB::ndbActionStrValid(QString const& actStr) {
    return (!actStr.compare("enable") || !actStr.compare("disable") || !actStr.compare("toggle"));
}

/*!
 * \brief Connect to WiFi network.
 * 
 * Note, this is the same as 'autoconnect' option from NickelMenu
 */
void NDB::wfmConnectWireless() {
    NDB_DBUS_USB_ASSERT((void) 0);
    return ndbWireless("autoconnect");
}

/*!
 * \brief Connect silently to WiFi network.
 * 
 * Note, this is the same as 'autoconnect_silent' from NickelMenu
 */
void NDB::wfmConnectWirelessSilently() {
    NDB_DBUS_USB_ASSERT((void) 0);
    return ndbWireless("autoconnect_silent");
}

/*!
 * \brief Enable/disable/toggle WiFi. Same as NickelMenu WiFi 'enable'/'disable'/'toggle' options
 * 
 * Note, this is the same as NickelMenu WiFi 'enable'/'disable'/'toggle' options.
 * 
 * \a action should be one of \c {enable}, \c {disable}, \c {toggle}
 */
void NDB::wfmSetAirplaneMode(QString const& action) {
    NDB_DBUS_USB_ASSERT((void) 0);
    NDB_DBUS_ASSERT((void) 0, QDBusError::InvalidArgs, ndbActionStrValid(action), "invalid action name");
    QByteArray actBytes = action.toUtf8();
    return ndbWireless(actBytes.constData());
}

void NDB::ndbWireless(const char *act) {
    nm_action_result_t *res = nm_action_nickel_wifi(act);
    if (!res) {
        nh_log("ndbWireless failed with error: %s", nm_err_peek());
        sendErrorReply(QDBusError::InternalError, QString("ndbWireless failed with error: %1").arg(nm_err()));
        return;
    }
    nm_action_result_free(res);
}

/*!
 * \brief Open the web browser.
 * 
 * Note, this is the same as the NickelMenu browser options
 * 
 * Opens the web browser to the default homepage. if \a modal
 * is \c true then the browser will be opened as a modal box with
 * a close button. If \a url is set, the browser will open it on
 * open. If \a css is set, additional CSS is supplied to the browser
 */
void NDB::bwmOpenBrowser(bool modal, QString const& url, QString const& css) {
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

/*!
 * \brief Invert the screen
 * 
 * Set \a action to \c {enable}, \c {disable} or \c {toggle} inversion.
 */
void NDB::nsInvert(QString const& action) {
    NDB_DBUS_USB_ASSERT((void) 0);
    return ndbSettings(action, "invert");
}

/*!
 * \brief Set UnlockEnabled
 * 
 * Set \a action to \c {enable}, \c {disable} or \c {toggle} UnlockEnabled.
 */
void NDB::nsLockscreen(QString const& action) {
    NDB_DBUS_USB_ASSERT((void) 0);
    return ndbSettings(action, "lockscreen");
}

/*!
 * \brief Set screenshot setting
 * 
 * Set \a action to \c {enable}, \c {disable} or \c {toggle} screenshots.
 */
void NDB::nsScreenshots(QString const& action) {
    NDB_DBUS_USB_ASSERT((void) 0);
    return ndbSettings(action, "screenshots");
}

/*!
 * \brief Sets the developer ForceWifiOn setting
 * 
 * Set \a action to \c {enable}, \c {disable} or \c {toggle} ForceWifiOn.
 */
void NDB::nsForceWifi(QString const& action) {
    NDB_DBUS_USB_ASSERT((void) 0);
    return ndbSettings(action, "force_wifi");
}

/*!
 * \brief Sets the auto USB connect setting
 * 
 * Set \a action to \c {enable}, \c {disable} or \c {toggle} auto USB connect.
 */
void NDB::nsAutoUSBGadget(QString const& action) {
    NDB_DBUS_USB_ASSERT((void) 0);
    return ndbSettings(action, "auto_usb_gadget");
}

void NDB::ndbSettings(QString const& action, const char* setting) {
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

/*!
 * \brief Shutdown Kobo
 */
void NDB::pwrShutdown() {
    NDB_DBUS_USB_ASSERT((void) 0);
    return pwrAction("shutdown");
}

/*!
 * \brief Reboot Kobo
 */
void NDB::pwrReboot() {
    NDB_DBUS_USB_ASSERT((void) 0);
    return pwrAction("reboot");
}

void NDB::pwrAction(const char *action) {
    nm_action_result_t *res = nm_action_power(action);
    if (!res) {
        nh_log("pwrAction failed with error: %s", nm_err_peek());
        sendErrorReply(QDBusError::InternalError, QString("pwrAction failed with error: %1").arg(nm_err()));
        return;
    }
    nm_action_result_free(res);
}

/* Signal Documentation */

/*!
 * \fn void NDB::dlgConfirmResult(int result)
 * \brief The signal that is emitted when a confirmation dialog is dismissed
 * 
 * When emitted, \a result will be \c 1 for ACCEPT or \c 0 for REJECT
 */

/*!
 * \fn void NDB::pfmDoneProcessing()
 * \brief The signal that nickel emits when the content import process has completed.
 * 
 * The signal will be emitted following the content import triggered whenever 
 * the user unplugs from the computer, when \c rescan_books / \c rescan_books_full 
 * actions are triggered from NickelMenu, or when \l NDB::pfmRescanBooks() 
 * or \l NDB::pfmRescanBooksFull() methods are called from NDB.
 */

/*!
 * \fn void NDB::pfmAboutToConnect()
 * \brief The signal that nickel emits when it is about to start the USB connection
 */

/*!
 * \fn void NDB::wmTryingToConnect()
 * \brief (todo: figure this out)
 */

/*!
 * \fn void NDB::wmNetworkConnected()
 * \brief This signal appears to be emitted when the network has successfully connected
 * I'm unsure if this is emitted when the WiFi connects, or when a valid IP address
 * is obtained.
 */

/*!
 * \fn void NDB::wmNetworkDisconnected()
 * \brief (todo: figure this out)
 */

/*!
 * \fn void NDB::wmNetworkForgotten()
 * \brief (todo: figure this out)
 */

/*!
 * \fn void NDB::wmNetworkFailedToConnect()
 * \brief (todo: figure this out)
 */

/*!
 * \fn void NDB::wmScanningStarted()
 * \brief (todo: figure this out)
 */

/*!
 * \fn void NDB::wmScanningFinished()
 * \brief (todo: figure this out)
 */

/*!
 * \fn void NDB::wmScanningAborted()
 * \brief (todo: figure this out)
 */

/*!
 * \fn void NDB::wmWifiEnabled(bool enabled)
 * \brief (todo: figure this out)
 * 
 * Is wifi \a enabled ?
 */

/*!
 * \fn void NDB::wmLinkQualityForConnectedNetwork(double quality)
 * \brief (todo: figure this out)
 * 
 * Shows the \a quality of the wifi signal
 */

/*!
 * \fn void NDB::wmMacAddressAvailable(QString mac)
 * \brief (todo: figure this out)
 * 
 * \a mac address
 */

/*!
 * \fn void NDB::ndbViewChanged(QString newView)
 * \brief The signal that is emitted when the current view changes
 * 
 * \a newView is the class name of the new view.
 * 
 * \sa ndbCurrentView
 */
