#include <dlfcn.h>
#include <QApplication>
#include <QString>
#include <QWidget>
#include <QFrame>
#include <QRegExp>
#include <QStringList>
#include <unistd.h>
#include <string.h>
#include <NickelHook.h>
#include "../../NickelMenu/src/action.h"
#include "../../NickelMenu/src/util.h"
#include "util.h"
#include "NDBDbus.h"
#include "../interface/ndb_adapter.h"

namespace NDB {
/*!
 * \class NDBDbus
 * \inmodule NickelDBus
 * \brief The NDBDbus class registers a service on d-bus of Kobo e-readers.
 * 
 * NDBDbus provides a bridge between Kobo's proprietary software, libnickel, 
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
NDBDbus::NDBDbus(QObject* parent) : QObject(parent), QDBusContext() {
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
    NDB_RESOLVE_SYMBOL("_ZN11PlugManager14sharedInstanceEv", nh_symoutptr(nSym.PlugManager__sharedInstance));
    NDB_RESOLVE_SYMBOL("_ZNK11PlugManager10gadgetModeEv", nh_symoutptr(nSym.PlugManager__gadgetMode));
    if (!nSym.PlugManager__gadgetMode) {
        // Older firmware versions use a slightly different mangled symbol
        NDB_RESOLVE_SYMBOL("_ZN11PlugManager10gadgetModeEv", nh_symoutptr(nSym.PlugManager__gadgetMode));
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
    // Setup the Confirmation Dialog object
    cfmDlg = new NDBCfmDlg(this);
    if (!cfmDlg || cfmDlg->initResult == InitError) {
        nh_log("failed to create confirmation dialog object");
        initSucceeded = false;
        return;
    }
    // // Setup the N3 Dialog object
    // n3Dlg = new NDBN3Dlg(this);
    // if (!n3Dlg || n3Dlg->initResult == NDBN3Dlg::InitError) {
    //     nh_log("failed to create N3 Dialog object");
    //     initSucceeded = false;
    //     return;
    // }
    viewTimer->setSingleShot(true);
    QObject::connect(viewTimer, &QTimer::timeout, this, &NDBDbus::handleQSWTimer);

    // Resolve the rest of the Nickel symbols up-front
    // PlugWorkFlowManager
    NDB_RESOLVE_SYMBOL("_ZN19PlugWorkflowManager14sharedInstanceEv", nh_symoutptr(nSym.PlugWorkflowManager_sharedInstance));
    // WirelessManager
    NDB_RESOLVE_SYMBOL("_ZN15WirelessManager14sharedInstanceEv", nh_symoutptr(nSym.WirelesManager_sharedInstance));
    // Toast
    NDB_RESOLVE_SYMBOL("_ZN20MainWindowController14sharedInstanceEv", nh_symoutptr(nSym.MainWindowController_sharedInstance));
    NDB_RESOLVE_SYMBOL("_ZN20MainWindowController5toastERK7QStringS2_i", nh_symoutptr(nSym.MainWindowController_toast));
    // Get N3Dialog content
    NDB_RESOLVE_SYMBOL("_ZN8N3Dialog7contentEv", nh_symoutptr(nSym.N3Dialog__content));
    // Device (for FW version)
    NDB_RESOLVE_SYMBOL("_ZN6Device16getCurrentDeviceEv", nh_symoutptr(nSym.Device__getCurrentDevice));
    NDB_RESOLVE_SYMBOL("_ZNK6Device9userAgentEv", nh_symoutptr(nSym.Device__userAgent));
    // MWC views
    NDB_RESOLVE_SYMBOL("_ZNK20MainWindowController11currentViewEv", nh_symoutptr(nSym.MainWindowController_currentView));
    if (!nSym.MainWindowController_currentView) {
        // Older firmware versions use a slightly different mangled symbol name
        NDB_RESOLVE_SYMBOL("_ZN20MainWindowController11currentViewEv", nh_symoutptr(nSym.MainWindowController_currentView));
    }
}

/*!
 * \internal
 * \brief Destroy the NDBDbus::NDBDbus object
 */
NDBDbus::~NDBDbus() {
    delete viewTimer;
    conn.unregisterService(NDB_DBUS_IFACE_NAME);
    conn.unregisterObject(NDB_DBUS_OBJECT_PATH);
}

template <typename T>
void NDBDbus::ndbConnectSignal(T *srcObj, const char *srcSignal, const char *dest) {
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
void NDBDbus::connectSignals() {
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
QString NDBDbus::ndbVersion() {
    return QStringLiteral(NH_VERSION);
}

/*!
 * \internal
 * \brief Set stackedWidget pointer to null if widget ever destroyed by Nickel
 */
void NDBDbus::handleStackedWidgetDestroyed() {
    stackedWidget = nullptr;
}

/*!
 * \internal
 * \brief Handle the QStackedWidget currentChanged event
 * 
 * \a index is the index of the new widget.
 */
void NDBDbus::handleQSWCurrentChanged(int index) {
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

/*!
 * \internal
 * \brief Emits ndbViewChanged() after a small timeout
 */
void NDBDbus::handleQSWTimer() {
    emit ndbViewChanged(ndbCurrentView());
}

/*!
 * \brief Get the class name of the current view.
 * 
 * Some class name examples are \c HomePageView \c ReadingView
 * among others.
 */
QString NDBDbus::ndbCurrentView() {
    QString name = QString();
    NDB_DBUS_SYM_ASSERT(name, nSym.MainWindowController_sharedInstance);
    NDB_DBUS_SYM_ASSERT(name, nSym.MainWindowController_currentView);
    MainWindowController *mwc = nSym.MainWindowController_sharedInstance();
    NDB_DBUS_ASSERT(name, QDBusError::InternalError, mwc, "unable to get shared MainWindowController instance");
    QWidget *cv = nSym.MainWindowController_currentView(mwc);
    NDB_DBUS_ASSERT(name, QDBusError::InternalError, cv, "unable to get current view from MainWindowController");
    if (!stackedWidget) {
        if (QString(cv->parentWidget()->metaObject()->className()) == "QStackedWidget") {
            stackedWidget = static_cast<QStackedWidget*>(cv->parentWidget());
            QObject::connect(stackedWidget, &QStackedWidget::currentChanged, this, &NDBDbus::handleQSWCurrentChanged);
            QObject::connect(stackedWidget, &QObject::destroyed, this, &NDBDbus::handleStackedWidgetDestroyed);
        } else {
            nh_log("expected QStackedWidget, got %s", cv->parentWidget()->metaObject()->className());
        }
    }
    name = cv->objectName();
    if (name == "N3Dialog") {
        NDB_DBUS_SYM_ASSERT(name, nSym.N3Dialog__content);
        if (QWidget *c = nSym.N3Dialog__content(cv)) {
            name = c->objectName();
        }
    } else if (name == "ReadingView") {
        rvConnectSignals(cv);
    }
    return name;
}

bool NDBDbus::ndbInUSBMS() {
    return nSym.PlugManager__gadgetMode(nSym.PlugManager__sharedInstance());
}

QString NDBDbus::getNickelMetaObjectDetails(const QMetaObject* nmo) {
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
QString NDBDbus::ndbNickelClassDetails(QString const& staticMetaobjectSymbol) {
    NDB_DBUS_USB_ASSERT("");
    typedef QMetaObject NickelMetaObject;
    NDB_DBUS_ASSERT("",QDBusError::InvalidArgs, staticMetaobjectSymbol.endsWith(QStringLiteral("staticMetaObjectE")), "not a valid staticMetaObject symbol");
    QByteArray sym = staticMetaobjectSymbol.toLatin1();
    NickelMetaObject *nmo;
    reinterpret_cast<void*&>(nmo) = dlsym(libnickel, sym.constData());
    NDB_DBUS_ASSERT("", QDBusError::InternalError, nmo, "could not dlsym staticMetaObject function for symbol %s", sym.constData());
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
bool NDBDbus::ndbSignalConnected(QString const &signalName) {
    return connectedSignals.contains(signalName);
}

/*!
 * \internal
 * \brief Print details gleaned from the QApplication instance
 */
QString NDBDbus::ndbNickelWidgets() {
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
 * \brief Get the current firmware version
 * 
 * Get the current firmware version as found in the user agent string
 */
QString NDBDbus::ndbFirmwareVersion() {
    NDB_DBUS_USB_ASSERT(fwVersion);
    if (fwVersion.isEmpty()) {
        NDB_DBUS_SYM_ASSERT(fwVersion, nSym.Device__getCurrentDevice);
        NDB_DBUS_SYM_ASSERT(fwVersion, nSym.Device__userAgent);
        Device *d = nSym.Device__getCurrentDevice();
        NDB_DBUS_ASSERT(fwVersion, QDBusError::InternalError, d, "unable to get current device");
        QString ua = QString::fromUtf8(nSym.Device__userAgent(d));
        QRegExp fwRegex = QRegExp("^.+\\(Kobo Touch (\\d+)/([\\d\\.]+)\\)$");
        NDB_DBUS_ASSERT(fwVersion, QDBusError::InternalError, (fwRegex.indexIn(ua) != -1 && fwRegex.captureCount() == 2), "could not get fw version from ua string");
        fwVersion = fwRegex.cap(2);
    }
    return fwVersion;
}

#define NDB_DLG_ASSERT(ret, cond) NDB_DBUS_ASSERT(ret, QDBusError::InternalError, cond, (cfmDlg->errString.toUtf8().constData()))

/*!
 * \brief Show a confirmation dialog with no buttons (except close)
 * 
 * Create a dialog box with \a title and \a body. This dialog only has a close
 * button.
 * 
 * When the dialog is closed, a \l dlgConfirmResult() signal is emitted.
 */
void NDBDbus::dlgConfirmNoBtn(QString const& title, QString const& body) {
    NDB_DBUS_USB_ASSERT((void) 0);
    NDB_DLG_ASSERT((void) 0, (cfmDlg->createDialog(NDBCfmDlg::TypeStd, title, body, "", "", true) == Ok));
    QObject::connect(cfmDlg->dlg, &QDialog::finished, this, &NDBDbus::dlgConfirmResult);
    cfmDlg->showDialog();
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
void NDBDbus::dlgConfirmAccept(QString const& title, QString const& body, QString const& acceptText) {
    NDB_DBUS_USB_ASSERT((void) 0);
    NDB_DLG_ASSERT((void) 0, (cfmDlg->createDialog(NDBCfmDlg::TypeStd, title, body, acceptText, "", true) == Ok));
    QObject::connect(cfmDlg->dlg, &QDialog::finished, this, &NDBDbus::dlgConfirmResult);
    cfmDlg->showDialog();
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
void NDBDbus::dlgConfirmReject(QString const& title, QString const& body, QString const& rejectText) {
    NDB_DBUS_USB_ASSERT((void) 0);
    NDB_DLG_ASSERT((void) 0, (cfmDlg->createDialog(NDBCfmDlg::TypeStd, title, body, "", rejectText, true) == Ok));
    QObject::connect(cfmDlg->dlg, &QDialog::finished, this, &NDBDbus::dlgConfirmResult);
    cfmDlg->showDialog();
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
void NDBDbus::dlgConfirmAcceptReject(QString const& title, QString const& body, QString const& acceptText, QString const& rejectText) {
    NDB_DBUS_USB_ASSERT((void) 0);
    NDB_DLG_ASSERT((void) 0, (cfmDlg->createDialog(NDBCfmDlg::TypeStd, title, body, acceptText, rejectText, true) == Ok));
    QObject::connect(cfmDlg->dlg, &QDialog::finished, this, &NDBDbus::dlgConfirmResult);
    cfmDlg->showDialog();
}

/*!
 * \brief Show confirmation dialog without buttons that can only be closed with close button, or programatically
 * 
 * Create a dialog box with \a title and \a body. This dialog has a close
 * button for safety, but is expected to be closed by \l dlgConfirmClose()
 * 
 * No signal is emitted when the dialog is closed
 * 
 * \since v0.2.0
 */
void NDBDbus::dlgConfirmModalMessage(QString const& title, QString const& body) {
    NDB_DBUS_USB_ASSERT((void) 0);
    NDB_DLG_ASSERT((void) 0, (cfmDlg->createDialog(NDBCfmDlg::TypeStd, title, body, "", "", false) == Ok));
    QObject::connect(cfmDlg->dlg, &QDialog::finished, this, &NDBDbus::dlgConfirmResult);
    cfmDlg->showDialog();
}

/*!
 * \brief Change body text of currently displayed dialog
 * 
 * Set the body text of the currently displayed dialog to \a body
 * replacing the existing body text.
 * 
 * There will be an error if the user has closed the dialog.
 * 
 * \since v0.2.0
 */ 
void NDBDbus::dlgConfirmChangeBody(QString const& body) {
    NDB_DBUS_USB_ASSERT((void) 0);
    NDB_DLG_ASSERT((void) 0, (cfmDlg->updateBody(body) == Ok));
}

/*!
 * \brief Close the currently opened dialog
 * 
 * Closes the currently open dialog. Will return an
 * error if the dialog has already been closed by the user.
 * 
 * \since v0.2.0
 */
void NDBDbus::dlgConfirmClose() {
    NDB_DBUS_USB_ASSERT((void) 0);
    NDB_DLG_ASSERT((void) 0, (cfmDlg->closeDialog() == Ok));
}

/*!
 * \internal
 * \brief slot for handling a line edit dialog that is accepted.
*/
void NDBDbus::onDlgLineEditAccepted() {
    emit dlgConfirmTextInput(cfmDlg->getText());
    emit dlgConfirmResult(QDialog::Accepted);
}

/*!
 * \internal
 * \brief slot for handling a line edit dialog that is rejected.
*/
void NDBDbus::onDlgLineEditRejected() {
    emit dlgConfirmTextInput("");
    emit dlgConfirmResult(QDialog::Rejected);
}

void NDBDbus::dlgConfirmLineEditFull(QString const& title, QString const& acceptText, QString const& rejectText, bool isPassword, QString const& setText) {
    NDB_DBUS_USB_ASSERT((void) 0);
    NDB_DLG_ASSERT((void) 0, (cfmDlg->createDialog(NDBCfmDlg::TypeLineEdit, title, "", acceptText, rejectText, true) == Ok));
    cfmDlg->setText(setText);
    cfmDlg->setPassword(isPassword);
    QObject::connect(cfmDlg->dlg, &QDialog::accepted, this, &NDBDbus::onDlgLineEditAccepted);
    QObject::connect(cfmDlg->dlg, &QDialog::rejected, this, &NDBDbus::onDlgLineEditRejected);
    cfmDlg->showDialog();
}

/*!
 * \brief Create dialog with single line text input
 * 
 * The dialog box will have a \a title, \a acceptText and \a rejectText buttons 
 * and a single line text entry field. If \a isPassword is \c {true}, the
 * text entry field will have its entry characters masked.
 * 
 * If the dialog is closed by tapping the 'accept' button, the 
 * \l dlgConfirmTextInput() signal will emit the contents of the text edit field 
 * (which may be an empty string), and \l dlgConfirmResult() will emit \c 1. 
 * Otherwise, \l dlgConfirmResult() will emit the result of \c 0 and \l dlgConfirmTextInput()
 * will emit an empty string.
 * 
 * \since v0.2.0
 */
void NDBDbus::dlgConfirmLineEdit(QString const& title, QString const& acceptText, QString const& rejectText, bool isPassword) {
    dlgConfirmLineEditFull(title, acceptText, rejectText, isPassword, "");
}

/*!
 * \brief Create dialog with single line text input with placeholder text
 * 
 * This is the same as \l dlgConfirmLineEdit() with the addition that placeholder
 * text will be added to the text entry field. This placeholder text is set to \a setText
 * 
 * The parameters \a title, \a acceptText, \a rejectText and \a isPassword are all the
 * same as \l dlgConfirmLineEdit()
 * 
 * \since v0.2.0
 */
void NDBDbus::dlgConfirmLineEditPlaceholder(QString const& title, QString const& acceptText, QString const& rejectText, bool isPassword, QString const& setText) {
    dlgConfirmLineEditFull(title, acceptText, rejectText, isPassword, setText);
}

/*!
 * \brief Create advanced dialog which can have widgets added
 * 
 * This creates an empty confirmation dialog with a \a title which you can add widgets to using the
 * \c dlgConfirmAdvancedAdd* methods. This will not be shown to the user until
 * the \l dlgConfirmAdvancedShow() method is called. The \c Accept and \c Reject buttons are set
 * with \a acceptText and \a rejectText parameters respectively. By default, widgets are stacked 
 * vertically. \l dlgConfirmAdvancedAddLayout() provides more advanced layout options.
 * 
 * If the user presses the accept button, \l dlgConfirmAdvancedJSON() will emit a JSON object. The widget
 * names will be used as keys, and the value will be the value of the widget when the accept button
 * is pressed. \l dlgConfirmResult() will emit the result of \c 1.
 * 
 * \l dlgConfirmResult() will emit the result of \c 0 if the reject button is pressed, and 
 * \l dlgConfirmAdvancedJSON() will emit an empty JSON object.
 * 
 * \since v0.2.0
 */
void NDBDbus::dlgConfirmAdvancedCreate(QString const& title, QString const& acceptText, QString const& rejectText) {
    NDB_DBUS_USB_ASSERT((void) 0);
    NDB_DLG_ASSERT((void) 0, (cfmDlg->createDialog(NDBCfmDlg::TypeAdvanced, title, "", acceptText, rejectText, true) == Ok));
}

/*!
 * \brief Add and begin using a new layout type in an advanced dialog
 * 
 * You can use this function for more advanced widget layout options compared to the default 
 * vertical box layout. \a layout can be one of three options: \c "horizontal", \c "vertical" 
 * and \c "form". You can add multiple layouts to a dialog as needed. Simply call this function,
 * add the widgets for the current layout, then call this function again to start a new
 * layout.
 * 
 * \since v0.2.0
 */
void NDBDbus::dlgConfirmAdvancedAddLayout(QString const& layout) {
    auto l = layout.toLower();
    NDB_DBUS_ASSERT((void) 0, QDBusError::InvalidArgs, (l == "vertical" || l == "horizontal" || l == "form"), 
                    "Parameter must be one of 'vertical', 'horizontal' or 'form'");
    enum NDBCfmDlg::layoutType lt;
    if (l == "vertical")
        lt = NDBCfmDlg::VertLayout;
    else if (l == "horizontal")
        lt = NDBCfmDlg::HorLayout;
    else 
        lt = NDBCfmDlg::FormLayout;
    NDB_DLG_ASSERT((void) 0, (cfmDlg->advAddLayout(lt) == Ok));
}

/*!
 * \brief Add a checkbox to an advanced dialog
 * 
 * This adds a checkbox to an advanced dialog. The \a name of the checkbox is used to retrieve the value
 * later. The user visible text is set by \a label and you can set the checkbox to be \a checked or not.
 * 
 * \since v0.2.0
 */
void NDBDbus::dlgConfirmAdvancedAddCheckBox(QString const& name, QString const& label, bool checked) {
    NDB_DBUS_USB_ASSERT((void) 0);
    NDB_DLG_ASSERT((void) 0, (cfmDlg->advAddCheckbox(name, label, checked) == Ok));
}

/*!
 * \brief Add a slider to an advanced dialog
 * 
 * This adds a slider to an advanced dialog. The \a name of the the slider is used to retrieve the value later.
 * The user visible text is set by \a label. The slider range is set by \a min and \a max and the starting position
 * is set by \a val.
 * 
 * \since v0.2.0
 */
void NDBDbus::dlgConfirmAdvancedAddSlider(QString const& name, QString const& label, int min, int max, int val) {
    NDB_DBUS_USB_ASSERT((void) 0);
    NDB_DLG_ASSERT((void) 0, (cfmDlg->advAddSlider(name, label, min, max, val) == Ok));
}

/*!
 * \brief Add a slider to an advanced dialog
 * 
 * This adds a dropdown to an advanced dialog. The \a name of the the slider is used to dropdown the value later.
 * The user visible text is set by \a label. The dropdown list is populated by \a items. You can enable addition
 * and removal of items using \a allowAdditionAndRemoval.
 * 
 * \since v0.2.0
 */
void NDBDbus::dlgConfirmAdvancedAddDropdown(QString const& name, QString const& label, QStringList items) {
    NDB_DBUS_USB_ASSERT((void) 0);
    NDB_DLG_ASSERT((void) 0, (cfmDlg->advAddDropDown(name, label, items) == Ok));
}

/*!
 * \brief Add a text line edit to an advanced dialog
 * 
 * This adds a text line edit to an advanced dialog. The \a name of the line edit is used to retrieve the value later.
 * The user visible label is set by \a label. Setting the optional parameter \a autoFormatCaps to \c false will disable 
 * auto capitalization of the first letter in the line. It defaults to true.
 * 
 * \since v0.2.0
 */
void NDBDbus::dlgConfirmAdvancedAddLineEdit(QString const& name, QString const& label, bool autoFormatCaps) {
    NDB_DBUS_USB_ASSERT((void) 0);
    NDB_DLG_ASSERT((void) 0, (cfmDlg->advAddLineEdit(name, label, autoFormatCaps) == Ok));
}

/*!
 * \brief Add a multiline text edit field to an advanced dialog
 * 
 * This adds a text line edit to an advanced dialog. The \a name of the line edit is used to retrieve the value later.
 * The user visible label is set by \a label. Setting the optional parameter \a autoFormatCaps to \c false will disable 
 * auto capitalization of the first letter in the line. It defaults to true.
 * 
 * \since v0.2.0
 */
void NDBDbus::dlgConfirmAdvancedAddTextEdit(QString const& name, QString const& label, bool autoFormatCaps) {
    NDB_DBUS_USB_ASSERT((void) 0);
    NDB_DLG_ASSERT((void) 0, (cfmDlg->advAddTextEdit(name, label, autoFormatCaps) == Ok));
}

/*!
 * \brief Add a date picker to an advanced dialog
 * 
 * This adds a date picker to an advanced dialog. The \a name of the date picker is used to retrieve the date later.
 * The user visible label is set by \a label. The picker will display the current date.
 * 
 * \since v0.2.0
 */
void NDBDbus::dlgConfirmAdvancedAddDatePicker(QString const& name, QString const& label) {
    NDB_DBUS_USB_ASSERT((void) 0);
    NDB_DLG_ASSERT((void) 0, (cfmDlg->advAddDatePicker(name, label, QDate::currentDate()) == Ok));
}

/*!
 * \brief Add a date picker to an advanced dialog
 * 
 * This adds a date picker to an advanced dialog. The \a name of the date picker is used to retrieve the date later.
 * The user visible label is set by \a label. You can use \a init to set the initial date. \a init must 
 * use the format \c yyyy-mm-dd.
 * 
 * \since v0.2.0
 */
void NDBDbus::dlgConfirmAdvancedAddDatePicker(QString const& name, QString const& label, QString const& init) {
    NDB_DBUS_USB_ASSERT((void) 0);
    auto d = QDate::fromString(init, Qt::ISODate);
    NDB_DBUS_ASSERT((void) 0, QDBusError::InvalidArgs, d.isValid(), "invalid init date");
    NDB_DLG_ASSERT((void) 0, (cfmDlg->advAddDatePicker(name, label, d) == Ok));
}

/*!
 * \brief Add a time picker to an advanced dialog
 * 
 * This adds a time picker to an advanced dialog. The \a name of the time picker is used to retrieve the time later.
 * The user visible label is set by \a label.
 * 
 * \since v0.2.0
 */
void NDBDbus::dlgConfirmAdvancedAddTimePicker(QString const& name, QString const& label) {
    NDB_DBUS_USB_ASSERT((void) 0);
    NDB_DLG_ASSERT((void) 0, (cfmDlg->advAddTimePicker(name, label, QTime::currentTime()) == Ok));
}

/*!
 * \brief Add a time picker to an advanced dialog
 * 
 * This adds a time picker to an advanced dialog. The \a name of the time picker is used to retrieve the time later.
 * The user visible label is set by \a label. You can use \a init to set the initial time. \a init must 
 * use the format \c hh:ss in 24-hr time.
 * 
 * \since v0.2.0
 */
void NDBDbus::dlgConfirmAdvancedAddTimePicker(QString const& name, QString const& label, QString const& init) {
    NDB_DBUS_USB_ASSERT((void) 0);
    auto t = QTime::fromString(init, "HH:mm");
    NDB_DBUS_ASSERT((void) 0, QDBusError::InvalidArgs, t.isValid(), "invalid init time");
    NDB_DLG_ASSERT((void) 0, (cfmDlg->advAddTimePicker(name, label, t) == Ok));
}

/*!
 * \brief Show an advanced dialog
 * 
 * An advanced dialog MUST be created with \l dlgConfirmAdvancedCreate() first.
 * 
 * \since v0.2.0
 */
void NDBDbus::dlgConfirmAdvancedShow() {
    NDB_DBUS_USB_ASSERT((void) 0);
    QObject::connect(cfmDlg->dlg, &QDialog::accepted, this, &NDBDbus::onAdvancedDlgAccepted);
    QObject::connect(cfmDlg->dlg, &QDialog::rejected, this, &NDBDbus::onAdvancedDlgRejected);
    NDB_DLG_ASSERT((void) 0, (cfmDlg->showDialog() == Ok));
}

/*!
 * \internal
 * \brief Slot for handling when the accept button for the Advanced confirmation dialog is pressed
 */
void NDBDbus::onAdvancedDlgAccepted() {
    QString json;
    if (cfmDlg->advGetJSON(json) == Ok) {
        emit dlgConfirmAdvancedJSON(json);
    } else {
        emit dlgConfirmAdvancedJSON("{}");
    }
    emit dlgConfirmResult(QDialog::Accepted);
}

/*!
 * \internal
 * \brief Slot for handling when the reject or close button for the Advanced confirmation dialog is pressed
 */
void NDBDbus::onAdvancedDlgRejected() {
    emit dlgConfirmAdvancedJSON("{}");
    emit dlgConfirmResult(QDialog::Rejected);
}

/*!
 * \brief Show a small, temporary text box with a message 
 * 
 * Show a text box on screen for \a toastDuration duration (in milliseconds)
 * with \a msgMain as the body text, and an optional \a msgSub
 */
void NDBDbus::mwcToast(int toastDuration, QString const &msgMain, QString const &msgSub) {
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
void NDBDbus::mwcHome() {
    NDB_DBUS_USB_ASSERT((void) 0);
    return ndbNickelMisc("home");
}

/*!
 * \brief Begin an abbreviated book rescan. Same as 'rescan_books' from NickelMenu
 */
void NDBDbus::pfmRescanBooks() {
    NDB_DBUS_USB_ASSERT((void) 0);
    return ndbNickelMisc("rescan_books");
}

/*!
 * \brief Begins a full book rescan. Same as 'rescan_books_full' from NickelMenu
 */
void NDBDbus::pfmRescanBooksFull() {
    NDB_DBUS_USB_ASSERT((void) 0);
    return ndbNickelMisc("rescan_books_full");
}

void NDBDbus::ndbNickelMisc(const char *action) {
    nm_action_result_t *res = nm_action_nickel_misc(action);
    if (!res) {
        nh_log("nm_action_nickel_misc failed with error: %s", nm_err_peek());
        sendErrorReply(QDBusError::InternalError, QString("nm_action_nickel_misc failed with error: %1").arg(nm_err()));
        return;
    }
    nm_action_result_free(res);
}

bool NDBDbus::ndbActionStrValid(QString const& actStr) {
    return (!actStr.compare("enable") || !actStr.compare("disable") || !actStr.compare("toggle"));
}

/*!
 * \brief Connect to WiFi network.
 * 
 * Note, this is the same as 'autoconnect' option from NickelMenu
 */
void NDBDbus::wfmConnectWireless() {
    NDB_DBUS_USB_ASSERT((void) 0);
    return ndbWireless("autoconnect");
}

/*!
 * \brief Connect silently to WiFi network.
 * 
 * Note, this is the same as 'autoconnect_silent' from NickelMenu
 */
void NDBDbus::wfmConnectWirelessSilently() {
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
void NDBDbus::wfmSetAirplaneMode(QString const& action) {
    NDB_DBUS_USB_ASSERT((void) 0);
    NDB_DBUS_ASSERT((void) 0, QDBusError::InvalidArgs, ndbActionStrValid(action), "invalid action name");
    QByteArray actBytes = action.toUtf8();
    return ndbWireless(actBytes.constData());
}

void NDBDbus::ndbWireless(const char *act) {
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
void NDBDbus::bwmOpenBrowser(bool modal, QString const& url, QString const& css) {
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
void NDBDbus::nsInvert(QString const& action) {
    NDB_DBUS_USB_ASSERT((void) 0);
    return ndbSettings(action, "invert");
}

/*!
 * \brief Set UnlockEnabled
 * 
 * Set \a action to \c {enable}, \c {disable} or \c {toggle} UnlockEnabled.
 */
void NDBDbus::nsLockscreen(QString const& action) {
    NDB_DBUS_USB_ASSERT((void) 0);
    return ndbSettings(action, "lockscreen");
}

/*!
 * \brief Set screenshot setting
 * 
 * Set \a action to \c {enable}, \c {disable} or \c {toggle} screenshots.
 */
void NDBDbus::nsScreenshots(QString const& action) {
    NDB_DBUS_USB_ASSERT((void) 0);
    return ndbSettings(action, "screenshots");
}

/*!
 * \brief Sets the developer ForceWifiOn setting
 * 
 * Set \a action to \c {enable}, \c {disable} or \c {toggle} ForceWifiOn.
 */
void NDBDbus::nsForceWifi(QString const& action) {
    NDB_DBUS_USB_ASSERT((void) 0);
    return ndbSettings(action, "force_wifi");
}

/*!
 * \brief Sets the auto USB connect setting
 * 
 * Set \a action to \c {enable}, \c {disable} or \c {toggle} auto USB connect.
 */
void NDBDbus::nsAutoUSBGadget(QString const& action) {
    NDB_DBUS_USB_ASSERT((void) 0);
    return ndbSettings(action, "auto_usb_gadget");
}

void NDBDbus::ndbSettings(QString const& action, const char* setting) {
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
void NDBDbus::pwrShutdown() {
    NDB_DBUS_USB_ASSERT((void) 0);
    return pwrAction("shutdown");
}

/*!
 * \brief Reboot Kobo
 */
void NDBDbus::pwrReboot() {
    NDB_DBUS_USB_ASSERT((void) 0);
    return pwrAction("reboot");
}

void NDBDbus::pwrAction(const char *action) {
    nm_action_result_t *res = nm_action_power(action);
    if (!res) {
        nh_log("pwrAction failed with error: %s", nm_err_peek());
        sendErrorReply(QDBusError::InternalError, QString("pwrAction failed with error: %1").arg(nm_err()));
        return;
    }
    nm_action_result_free(res);
}

void NDBDbus::rvConnectSignals(QWidget* rv) {
    // Just connecting pageChanged(int) for now. Others may or may not 
    // come in the future.
    QObject::connect(rv, SIGNAL(pageChanged(int)), this, SIGNAL(rvPageChanged(int)), Qt::UniqueConnection);
}

/* Signal Documentation */

/*!
 * \fn void NDBDbus::dlgConfirmResult(int result)
 * \brief The signal that is emitted when a confirmation dialog is dismissed
 * 
 * When emitted, \a result will be \c 1 for ACCEPT or \c 0 for REJECT
 */

/*!
 * \fn void NDBDbus::dlgConfirmTextInput(QString input)
 * \brief The signal that is emitted when text is entered by user
 * 
 * When emitted \a input will be the text the user inputted. This signal is
 * only emitted when the user taps the \c accept button is tapped. \a input 
 * may be an empty string.
 * 
 * \since v0.2.0
 */

/*!
 * \fn void NDBDbus::dlgConfirmAdvancedJSON(QString json)
 * \brief The signal that is emitted when the user accepts an advanced confirmation dialog
 * 
 * When emitted, \a json will be a JSON object with the keys set to the widget names, and
 * the value will be set to the value of each widget at the time the accept button was pressed.
 * 
 * \since v0.2.0
 */

/*!
 * \fn void NDBDbus::pfmDoneProcessing()
 * \brief The signal that nickel emits when the content import process has completed.
 * 
 * The signal will be emitted following the content import triggered whenever 
 * the user unplugs from the computer, when \c rescan_books / \c rescan_books_full 
 * actions are triggered from NickelMenu, or when \l NDBDbus::pfmRescanBooks() 
 * or \l NDBDbus::pfmRescanBooksFull() methods are called from NDBDbus.
 */

/*!
 * \fn void NDBDbus::pfmAboutToConnect()
 * \brief The signal that nickel emits when it is about to start the USB connection
 */

/*!
 * \fn void NDBDbus::wmTryingToConnect()
 * \brief (todo: figure this out)
 */

/*!
 * \fn void NDBDbus::wmNetworkConnected()
 * \brief This signal appears to be emitted when the network has successfully connected
 * I'm unsure if this is emitted when the WiFi connects, or when a valid IP address
 * is obtained.
 */

/*!
 * \fn void NDBDbus::wmNetworkDisconnected()
 * \brief (todo: figure this out)
 */

/*!
 * \fn void NDBDbus::wmNetworkForgotten()
 * \brief (todo: figure this out)
 */

/*!
 * \fn void NDBDbus::wmNetworkFailedToConnect()
 * \brief (todo: figure this out)
 */

/*!
 * \fn void NDBDbus::wmScanningStarted()
 * \brief (todo: figure this out)
 */

/*!
 * \fn void NDBDbus::wmScanningFinished()
 * \brief (todo: figure this out)
 */

/*!
 * \fn void NDBDbus::wmScanningAborted()
 * \brief (todo: figure this out)
 */

/*!
 * \fn void NDBDbus::wmWifiEnabled(bool enabled)
 * \brief (todo: figure this out)
 * 
 * Is wifi \a enabled ?
 */

/*!
 * \fn void NDBDbus::wmLinkQualityForConnectedNetwork(double quality)
 * \brief (todo: figure this out)
 * 
 * Shows the \a quality of the wifi signal
 */

/*!
 * \fn void NDBDbus::wmMacAddressAvailable(QString mac)
 * \brief (todo: figure this out)
 * 
 * \a mac address
 */

/*!
 * \fn void NDBDbus::ndbViewChanged(QString newView)
 * \brief The signal that is emitted when the current view changes
 * 
 * This signal is only emitted if \l NDBDbus::ndbCurrentView() has been called 
 * at least once by an application. \a newView is the class name of the new view.
 * 
 * \sa NDBDbus::ndbCurrentView()
 */

/*!
 * \fn void NDBDbus::rvPageChanged(int pageNum)
 * \brief The signal that is emitted when the current book changes page
 * 
 * This signal is only emitted if \l NDBDbus::ndbCurrentView() has been called 
 * at least once by an application. \a pageNum is kepub or epub page 
 * number of the new page.
 * 
 * \sa NDBDbus::ndbCurrentView()
 */

} // namespace NDB
