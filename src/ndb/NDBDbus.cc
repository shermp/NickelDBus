#include <dlfcn.h>
#include <QApplication>
#include <QString>
#include <QWidget>
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

/*!
 * \namespace NDB
 * 
 * \brief Contains classes and methods for NickelDBus
 */
namespace NDB {
/*!
 * \class NDB::NDBDbus
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
    // Image
    NDB_RESOLVE_SYMBOL("_ZN5Image11sizeForTypeERK6DeviceRK7QString", nh_symoutptr(nSym.Image__sizeForType));
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
 * \internal
 * \brief Utility method to create one of the preset dialogs
 */
enum Result NDBDbus::dlgConfirmCreatePreset(QString const& title, QString const& body, QString const& acceptText, QString const& rejectText) {
    enum Result res;
    NDB_ASSERT_RES(res, cfmDlg->createDialog(NDBCfmDlg::TypeStd));
    if (!title.isEmpty()) { NDB_ASSERT_RES(res, cfmDlg->setTitle(title)); }
    if (!body.isEmpty()) { NDB_ASSERT_RES(res, cfmDlg->setBody(body)); }
    if (!rejectText.isEmpty()) { NDB_ASSERT_RES(res, cfmDlg->setReject(rejectText)); }
    if (!acceptText.isEmpty()) { NDB_ASSERT_RES(res, cfmDlg->setAccept(acceptText)); }
    QObject::connect(cfmDlg->dlg, &QDialog::finished, this, &NDBDbus::dlgConfirmResult);
    NDB_ASSERT_RES(res, cfmDlg->showDialog());
    return Ok;
}

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
    NDB_DLG_ASSERT((void) 0, (dlgConfirmCreatePreset(title, body, "", "") == Ok));
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
    NDB_DLG_ASSERT((void) 0, (dlgConfirmCreatePreset(title, body, acceptText, "") == Ok));
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
    NDB_DLG_ASSERT((void) 0, (dlgConfirmCreatePreset(title, body, "", rejectText) == Ok));
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
    NDB_DLG_ASSERT((void) 0, (dlgConfirmCreatePreset(title, body, acceptText, rejectText) == Ok));
}

/*!
 * \brief Create a flexible confirmation dialog
 * 
 * Create (but not show) a flexible confirmation dialog. If \a createLineEdit is
 * \c true a LineEdit dialog will be created, otherwise a standard dialog is created.
 * 
 * The caller can invoke \l dlgConfirmSetTitle(), \l dlgConfirmSetBody(),
 * \l dlgConfirmSetAccept(), \l dlgConfirmSetReject(), \l dlgConfirmSetModal(),
 * \l dlgConfirmShowClose(), \l dlgConfirmSetProgress() to customise the appearance and
 * behaviour of the dialog. If the dialog is a LineEdit, \l dlgConfirmSetLEPassword() and
 * \l dlgConfirmSetLEPlaceholder() can also be called.
 * 
 * Show the dialog by calling \l dlgConfirmShow(). It can be closed by calling 
 * \l dlgConfirmClose() in addition to the user closing it.
 * 
 * \l dlgConfirmResult() signal is emitted when the dialog is closed.
 *
 * For a LineEdit dialog, if the dialog is closed by tapping the 'accept' button, the 
 * \l dlgConfirmTextInput() signal will emit the contents of the text edit field 
 * (which may be an empty string), and \l dlgConfirmResult() will emit \c 1. 
 * Otherwise, \l dlgConfirmResult() will emit the result of \c 0 and \l dlgConfirmTextInput()
 * will emit an empty string.
 * 
 * \since 0.2.0
 */
void NDBDbus::dlgConfirmCreate(bool createLineEdit) {
    NDB_DBUS_USB_ASSERT((void) 0);
    NDB_DLG_ASSERT((void) 0, (cfmDlg->createDialog(createLineEdit ? NDBCfmDlg::TypeLineEdit : NDBCfmDlg::TypeStd) == Ok));
    if (createLineEdit) {
        QObject::connect(cfmDlg->dlg, &QDialog::accepted, this, &NDBDbus::onDlgLineEditAccepted);
        QObject::connect(cfmDlg->dlg, &QDialog::rejected, this, &NDBDbus::onDlgLineEditRejected);
    } else {
        QObject::connect(cfmDlg->dlg, &QDialog::finished, this, &NDBDbus::dlgConfirmResult);
    }
}

/*!
 * \brief Set title of an existing confirmation dialog
 *
 * The confirmation dialog will have the title set to \a title
 * 
 * \since 0.2.0
 */
void NDBDbus::dlgConfirmSetTitle(QString const& title) {
    NDB_DBUS_USB_ASSERT((void) 0);
    NDB_DLG_ASSERT((void) 0, (cfmDlg->setTitle(title) == Ok));
}

/*!
 * \brief Set body text of an existing confirmation dialog
 *
 * The confirmation dialog will have the body text set to \a body
 * 
 * \since 0.2.0
 */
void NDBDbus::dlgConfirmSetBody(QString const& body) {
    NDB_DBUS_USB_ASSERT((void) 0);
    NDB_DLG_ASSERT((void) 0, (cfmDlg->setBody(body) == Ok));
}

/*!
 * \brief Set the accept button of an existing confirmation dialog
 *
 * The accept button will be enabled, and its label will be set
 * to \a acceptText
 * 
 * \since 0.2.0
 */
void NDBDbus::dlgConfirmSetAccept(QString const& acceptText) {
    NDB_DBUS_USB_ASSERT((void) 0);
    NDB_DLG_ASSERT((void) 0, (cfmDlg->setAccept(acceptText) == Ok));
}

/*!
 * \brief Set the reject button of an existing confirmation dialog
 *
 * The reject button will be enabled, and its label will be set
 * to \a rejectText
 * 
 * \since 0.2.0
 */
void NDBDbus::dlgConfirmSetReject(QString const& rejectText) {
    NDB_DBUS_USB_ASSERT((void) 0);
    NDB_DLG_ASSERT((void) 0, (cfmDlg->setReject(rejectText) == Ok));
}

/*!
 * \brief Set whether the confirmation dialog will be modal
 *
 * If \a modal is \c true the user will not be able to exit
 * the dialog by tapping outside it.
 * 
 * \since 0.2.0
 */
void NDBDbus::dlgConfirmSetModal(bool modal) {
    NDB_DBUS_USB_ASSERT((void) 0);
    NDB_DLG_ASSERT((void) 0, (cfmDlg->setModal(modal) == Ok));
}

/*!
 * \brief Set whether the confirmation dialog will have a close button
 *
 * If \a show is \c false the show button will not be displayed.
 * Note, if the dialog is modal, and accept and reject buttons are
 * not set, the user will have no means of closing the dialog.
 * 
 * \since 0.2.0
 */
void NDBDbus::dlgConfirmShowClose(bool show) {
    NDB_DBUS_USB_ASSERT((void) 0);
    NDB_DLG_ASSERT((void) 0, (cfmDlg->showClose(show) == Ok));
}

/*!
 * \brief Display a progress bar on the currently open dialog
 * 
 * Displays a progress bar with a range from \a min to \a max, and sets the
 * current value to \a val. If any one of \a min \a max or \a val are set
 * to \c -1, the progress bar will be hidden.
 * 
 * If set, \a format determines how the label will be displayed. It uses
 * the same placeholders as a QProgressBar. The placeholders are \c %p for
 * percentage value, \c %v for current step, \c %m for last step. The default
 * if not set is \c %p%.
 * 
 * \since 0.2.0
 */
void NDBDbus::dlgConfirmSetProgress(int min, int max, int val, QString const& format) {
    NDB_DBUS_USB_ASSERT((void) 0);
    NDB_DLG_ASSERT((void) 0, (cfmDlg->setProgress(min, max, val, format) == Ok));
}

/*!
 * \brief Sets whether the current line edit dialog is a password dialog
 *
 * If \a password is \c true the dialog will have a 'show password' checkbox, and
 * input text will be masked if that checkbox is not checked.
 * 
 * \since 0.2.0
 */
void NDBDbus::dlgConfirmSetLEPassword(bool password) {
    NDB_DBUS_USB_ASSERT((void) 0);
    NDB_DLG_ASSERT((void) 0, (cfmDlg->setLEPassword(password) == Ok));
}

/*!
 * \brief Add placeholder text to a line edit dialog
 *
 * Set the line edit placeholder to \a placeholder
 * 
 * \since 0.2.0
 */
void NDBDbus::dlgConfirmSetLEPlaceholder(QString const& placeholder) {
    NDB_DBUS_USB_ASSERT((void) 0);
    NDB_DLG_ASSERT((void) 0, (cfmDlg->setLEPlaceholder(placeholder) == Ok));
}

/*!
 * \brief Display the current dialog
 * 
 * \since 0.2.0
 */
void NDBDbus::dlgConfirmShow() {
    NDB_DBUS_USB_ASSERT((void) 0);
    NDB_DLG_ASSERT((void) 0, (cfmDlg->showDialog() == Ok));
}

/*!
 * \brief Close the currently opened dialog
 * 
 * Closes the currently open dialog. Will return an
 * error if the dialog has already been closed by the user.
 * 
 * \since 0.2.0
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
    emit dlgConfirmTextInput(cfmDlg->getLEText());
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
 * \brief Set Dark Mode
 *
 * Set \a action to \c {enable}, \c {disable} or \c {toggle} dark mode
 */
void NDBDbus::nsDarkMode(QString const& action) {
    NDB_DBUS_USB_ASSERT((void) 0);
    return ndbSettings(action, "dark_mode");
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

/*!
 * \brief Put Kobo to sleep
 * 
 * \since 0.2.0
 */
void NDBDbus::pwrSleep() {
    NDB_DBUS_USB_ASSERT((void) 0);
    return pwrAction("sleep");
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

/*!
 * \brief Gets the image size for the device
 * 
 * Valid strings for \a type are \c N3_FULL , \c N3_LIBRARY_FULL , \c N3_LIBRARY_GRID
 * 
 * Returns a string in the form \c "width \c height"
 * 
 * \since 0.3.0
 */
QString NDBDbus::imgSizeForType(QString const& type) {
    QString default_ret("-1 -1");
    NDB_DBUS_USB_ASSERT(default_ret);
    NDB_DBUS_SYM_ASSERT(default_ret, nSym.Image__sizeForType);
    bool type_valid = (type == "N3_FULL" || type == "N3_LIBRARY_FULL" || type == "N3_LIBRARY_GRID");
    NDB_DBUS_ASSERT(default_ret, QDBusError::InvalidArgs, type_valid, "invalid type name. Must be one of N3_FULL, N3_LIBRARY_FULL, N3_LIBRARY_GRID");
    NDB_DBUS_SYM_ASSERT(default_ret, nSym.Device__getCurrentDevice);
    Device *d = nSym.Device__getCurrentDevice();
    NDB_DBUS_ASSERT(default_ret, QDBusError::InternalError, d, "unable to get current device");
    QSize img_size = nSym.Image__sizeForType(d, type);
    return QString("%1 %2").arg(img_size.width()).arg(img_size.height());
}

/* Enum Documentation */

/*!
 * \enum NDB::Result
 * 
 * This enum stores the result status of a function or method
 * 
 * \value   Ok
 *          The function returned successfully without errors
 * \value   NotImplemented
 *          The function has no implementation
 * \value   InitError
 *          There was an error initialising an object
 * \value   SymbolError
 *          There was an error resolving a Nickel symbol
 * \value   NullError
 *          An unexpected nullptr was encountered
 * \value   ForbiddenError
 *          A function was called when not allowed to do so
 * \value   ParamError
 *          There was an error with a function parameter
 * \value   ConnError
 *          There was a dbus connection error
 */

/* Signal Documentation */

/*!
 * \fn void NDB::NDBDbus::dlgConfirmResult(int result)
 * \brief The signal that is emitted when a confirmation dialog is dismissed
 * 
 * When emitted, \a result will be \c 1 for ACCEPT or \c 0 for REJECT
 */

/*!
 * \fn void NDB::NDBDbus::dlgConfirmTextInput(QString input)
 * \brief The signal that is emitted when text is entered by user
 * 
 * When emitted \a input will be the text the user inputted. This signal is
 * only emitted when the user taps the \c accept button is tapped. \a input 
 * may be an empty string.
 * 
 * \since 0.2.0
 */

/*!
 * \fn void NDB::NDBDbus::pfmDoneProcessing()
 * \brief The signal that nickel emits when the content import process has completed.
 * 
 * The signal will be emitted following the content import triggered whenever 
 * the user unplugs from the computer, when \c rescan_books / \c rescan_books_full 
 * actions are triggered from NickelMenu, or when \l NDB::NDBDbus::pfmRescanBooks() 
 * or \l NDB::NDBDbus::pfmRescanBooksFull() methods are called from NDBDbus.
 */

/*!
 * \fn void NDB::NDBDbus::pfmAboutToConnect()
 * \brief The signal that nickel emits when it is about to start the USB connection
 */

/*!
 * \fn void NDB::NDBDbus::wmTryingToConnect()
 * \brief (todo: figure this out)
 */

/*!
 * \fn void NDB::NDBDbus::wmNetworkConnected()
 * \brief This signal appears to be emitted when the network has successfully connected
 * I'm unsure if this is emitted when the WiFi connects, or when a valid IP address
 * is obtained.
 */

/*!
 * \fn void NDB::NDBDbus::wmNetworkDisconnected()
 * \brief (todo: figure this out)
 */

/*!
 * \fn void NDB::NDBDbus::wmNetworkForgotten()
 * \brief (todo: figure this out)
 */

/*!
 * \fn void NDB::NDBDbus::wmNetworkFailedToConnect()
 * \brief (todo: figure this out)
 */

/*!
 * \fn void NDB::NDBDbus::wmScanningStarted()
 * \brief (todo: figure this out)
 */

/*!
 * \fn void NDB::NDBDbus::wmScanningFinished()
 * \brief (todo: figure this out)
 */

/*!
 * \fn void NDB::NDBDbus::wmScanningAborted()
 * \brief (todo: figure this out)
 */

/*!
 * \fn void NDB::NDBDbus::wmWifiEnabled(bool enabled)
 * \brief (todo: figure this out)
 * 
 * Is wifi \a enabled ?
 */

/*!
 * \fn void NDB::NDBDbus::wmLinkQualityForConnectedNetwork(double quality)
 * \brief (todo: figure this out)
 * 
 * Shows the \a quality of the wifi signal
 */

/*!
 * \fn void NDB::NDBDbus::wmMacAddressAvailable(QString mac)
 * \brief (todo: figure this out)
 * 
 * \a mac address
 */

/*!
 * \fn void NDB::NDBDbus::ndbViewChanged(QString newView)
 * \brief The signal that is emitted when the current view changes
 * 
 * This signal is only emitted if \l NDB::NDBDbus::ndbCurrentView() has been called 
 * at least once by an application. \a newView is the class name of the new view.
 * 
 * \sa NDB::NDBDbus::ndbCurrentView()
 */

/*!
 * \fn void NDB::NDBDbus::rvPageChanged(int pageNum)
 * \brief The signal that is emitted when the current book changes page
 * 
 * This signal is only emitted if \l NDB::NDBDbus::ndbCurrentView() has been called 
 * at least once by an application. \a pageNum is kepub or epub page 
 * number of the new page.
 * 
 * \sa NDB::NDBDbus::ndbCurrentView()
 */

} // namespace NDB
