#ifndef NDB_DBUS_H
#define NDB_DBUS_H
#include <QDialog>
#include <QObject>
#include <QString>
#include <QSet>
#include <QStackedWidget>
#include <QtDBus>
#include <QDBusContext>
#include <QLabel>
#include <QTimer>
#include "NDBCfmDlg.h"
#include "NDBVolContent.h"

typedef void PlugManager;
typedef QObject PlugWorkflowManager;
typedef QObject WirelessManager;
typedef void MainWindowController;
typedef QDialog ConfirmationDialog;
typedef QWidget N3Dialog;
typedef void Device;

#ifndef NDB_DBUS_IFACE_NAME
    #define NDB_DBUS_IFACE_NAME "com.github.shermp.nickeldbus"
#endif
#ifndef NDB_DBUS_OBJECT_PATH
    #define NDB_DBUS_OBJECT_PATH "/nickeldbus"
#endif

namespace NDB {

class NDBDbus : public QObject, protected QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", NDB_DBUS_IFACE_NAME)

    QDBusConnection conn = QDBusConnection::systemBus();
    
    public:
        bool initSucceeded;
        NDBDbus(QObject* parent);
        ~NDBDbus();
        // bool registerDBus();
        void connectSignals();

    Q_SIGNALS:
        void dlgConfirmResult(int result);
        void dlgConfirmTextInput(QString input);
        // PlugworkFlowManager signals
        void pfmDoneProcessing();
        void pfmAboutToConnect();
        // WirelessManager signals
        void wmTryingToConnect();
        void wmNetworkConnected();
        void wmNetworkDisconnected();
        void wmNetworkForgotten();
        void wmNetworkFailedToConnect();
        void wmScanningStarted();
        void wmScanningFinished();
        void wmScanningAborted();
        void wmWifiEnabled(bool enabled);
        void wmLinkQualityForConnectedNetwork(double quality);
        void wmMacAddressAvailable(QString mac);
        void ndbViewChanged(QString newView);
        void rvPageChanged(int pageNum);

    public Q_SLOTS:
        QString ndbVersion();
        QString ndbNickelClassDetails(QString const& staticMmetaobjectSymbol);
        QString ndbNickelWidgets();
        QString ndbCurrentView();
        QString ndbFirmwareVersion();
        // Metadata
        QStringList ndbBookList();
        QString ndbMetaData(QString const& cID);
        // misc
        bool ndbSignalConnected(QString const& signalName);
        void mwcToast(int toastDuration, QString const& msgMain, QString const& msgSub = QStringLiteral(""));
        void mwcHome();
        // Confirmation Dialogs
        void dlgConfirmNoBtn(QString const& title, QString const& body);
        void dlgConfirmAccept(QString const& title, QString const& body, QString const& acceptText);
        void dlgConfirmReject(QString const& title, QString const& body, QString const& rejectText);
        void dlgConfirmAcceptReject(QString const& title, QString const& body, QString const& acceptText, QString const& rejectText);
        // Flexi dialogs
        void dlgConfirmCreate(bool createLineEdit = false);
        void dlgConfirmSetTitle(QString const& title);
        void dlgConfirmSetBody(QString const& body);
        void dlgConfirmSetAccept(QString const& acceptText);
        void dlgConfirmSetReject(QString const& rejectText);
        void dlgConfirmSetModal(bool modal);
        void dlgConfirmShowClose(bool show);
        void dlgConfirmSetProgress(int min, int max, int val, QString const& format = "");
        void dlgConfirmSetLEPassword(bool password);
        void dlgConfirmSetLEPlaceholder(QString const& placeholder);
        void dlgConfirmShow();
        void dlgConfirmClose();
        // PlugWorkFlowManager
        void pfmRescanBooks();
        void pfmRescanBooksFull();
        // Wireless methods (WirelessFlowManager)
        void wfmConnectWireless();
        void wfmConnectWirelessSilently();
        void wfmSetAirplaneMode(QString const& action);
        // Web Browser (BrowserWorkflowManager)
        void bwmOpenBrowser(bool modal = false, QString const& url = QString(), QString const& css = QString());
        // Nickel Settings
        void nsInvert(QString const& action);
        void nsLockscreen(QString const& action);
        void nsScreenshots(QString const& action);
        void nsForceWifi(QString const& action);
        void nsAutoUSBGadget(QString const& action);
        // Power commands
        void pwrShutdown();
        void pwrReboot();
        void pwrSleep();
    protected Q_SLOTS:
        void handleQSWCurrentChanged(int index);
        void handleQSWTimer();
        void handleStackedWidgetDestroyed();
        void onDlgLineEditAccepted();
        void onDlgLineEditRejected();
    private:
        void *libnickel;
        QSet<QString> connectedSignals;
        QStackedWidget *stackedWidget = nullptr;
        QString fwVersion;
        NDBCfmDlg *cfmDlg;
        NDBVolContent *volContent;
        //NDBN3Dlg *n3Dlg;
        struct {
            bool *(*PlugManager__gadgetMode)(PlugManager*);
            PlugManager *(*PlugManager__sharedInstance)();
            PlugWorkflowManager *(*PlugWorkflowManager_sharedInstance)();
            WirelessManager *(*WirelesManager_sharedInstance)();
            MainWindowController *(*MainWindowController_sharedInstance)();
            void (*MainWindowController_toast)(MainWindowController*, QString const&, QString const&, int);
            QWidget *(*MainWindowController_currentView)(MainWindowController*);
            QWidget* (*N3Dialog__content)(N3Dialog*);
            Device *(*Device__getCurrentDevice)();
            QByteArray (*Device__userAgent)(Device*);
        } nSym;
        QTimer *viewTimer;
        bool ndbInUSBMS();
        bool ndbActionStrValid(QString const& actStr);
        void ndbWireless(const char *act);
        void ndbSettings(QString const& action, const char* setting);
        void ndbNickelMisc(const char *action);
        QString getNickelMetaObjectDetails(const QMetaObject* nmo);
        template <typename T>
        void ndbConnectSignal(T *srcObj, const char *srcSignal, const char *dest);
        void pwrAction(const char *action);
        void rvConnectSignals(QWidget* rv);
        void dlgConfirmLineEditFull(QString const& title, QString const& acceptText, QString const& rejectText, bool isPassword, QString const& setText);
        enum Result dlgConfirmCreatePreset(QString const& title, QString const& body, QString const& acceptText, QString const& rejectText);
};

} // namespace NDB

#endif // NDB_DBUS_H
