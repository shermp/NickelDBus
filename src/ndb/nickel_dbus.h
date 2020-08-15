#ifndef NICKEL_DBUS_H
#define NICKEL_DBUS_H
#include <QDialog>
#include <QObject>
#include <QString>
#include <QSet>
#include <QtDBus>
#include <QDBusContext>

typedef void PlugManager;
typedef QObject PlugWorkflowManager;
typedef QObject WirelessManager;
typedef void MainWindowController;
typedef QDialog ConfirmationDialog;

#ifndef NDB_DBUS_IFACE_NAME
    #define NDB_DBUS_IFACE_NAME "com.github.shermp.nickeldbus"
#endif
#ifndef NDB_DBUS_OBJECT_PATH
    #define NDB_DBUS_OBJECT_PATH "/nickeldbus"
#endif


class NickelDBus : public QObject, protected QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", NDB_DBUS_IFACE_NAME)

    QDBusConnection conn = QDBusConnection::systemBus();
    
    public:
        bool initSucceeded;
        /*!
         * \brief Construct a new Nickel D-Bus object
         * 
         * \param parent 
         */
        NickelDBus(QObject* parent);
        /*!
         * \brief Destroy the Nickel D-Bus object
         * 
         */
        ~NickelDBus();
        // bool registerDBus();
        /*!
         * \brief Connects available Nickel signals to d-bus
         * 
         * Failures to connect a signal will stop execution, the failure will be logged
         * to syslog. 
         */
        void connectSignals();

    Q_SIGNALS:
        /*!
         * \brief The signal that is emitted when a confirmation dialog button is tapped, or the dialog is closed
         * 
         * \a result will be 0 for REJECT or 1 for ACCEPT
         */
        void dlgConfirmResult(int result);
        // PlugworkFlowManager signals
        /*!
         * \brief The signal that nickel emits when the content import process has completed.
         * 
         * The signal will be emitted following the content import triggered whenever 
         * the user unplugs from the computer, when rescan_books/rescan_books_full 
         * actions are triggered from NickelMenu, or when pfmRescanBooks/pfmRescanBooksFull
         * methods are called from NickelDBus.
         */
        void pfmDoneProcessing();
        /*!
         * \brief The signal that nickel emits when it is about to start the USB connection
         * 
         */
        void pfmAboutToConnect();
        // WirelessManager signals
        /*!
         * \brief (todo: figure this out)
         * 
         */
        void wmTryingToConnect();
        /*!
         * \brief This signal appears to be emitted when the network has successfully connected
         * 
         * I'm unsure if this is emitted when the WiFi connects, or when a valid IP address
         * is obtained.
         */
        void wmNetworkConnected();
        /*!
         * \brief (todo: figure this out)
         * 
         */
        void wmNetworkDisconnected();
        /*!
         * \brief (todo: figure this out)
         * 
         */
        void wmNetworkForgotten();
        /*!
         * \brief (todo: figure this out)
         * 
         */
        void wmNetworkFailedToConnect();
        /*!
         * \brief (todo: figure this out)
         * 
         */
        void wmScanningStarted();
        /*!
         * \brief (todo: figure this out)
         * 
         */
        void wmScanningFinished();
        /*!
         * \brief (todo: figure this out)
         * 
         */
        void wmScanningAborted();
        /*!
         * \brief (todo: figure this out)
         * 
         * Is wifi \a enabled
         */
        void wmWifiEnabled(bool enabled);
        /*!
         * \brief (todo: figure this out)
         * 
         * Shows the \a quality of the wifi signal
         */
        void wmLinkQualityForConnectedNetwork(double quality);
        /*!
         * \brief (todo: figure this out)
         * 
         * \a mac address
         */
        void wmMacAddressAvailable(QString mac);

    public Q_SLOTS:
        QString ndbVersion();
        QString miscNickelClassDetails(QString const& staticMmetaobjectSymbol);
        // misc
        bool miscSignalConnected(QString const& signalName);
        void mwcToast(int toastDuration, QString const& msgMain, QString const& msgSub = QStringLiteral(""));
        void mwcHome();
        // Confirmation Dialogs
        void dlgConfirmNoBtn(QString const& title, QString const& body);
        void dlgConfirmAccept(QString const& title, QString const& body, QString const& acceptText);
        void dlgConfirmReject(QString const& title, QString const& body, QString const& rejectText);
        void dlgConfirmAcceptReject(QString const& title, QString const& body, QString const& acceptText, QString const& rejectText);
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
    protected Q_SLOTS:
        void allowDialog();
    private:
        void *libnickel;
        bool allowDlg = true;
        QSet<QString> connectedSignals;
        
        struct {
            bool *(*PlugManager__gadgetMode)(PlugManager*);
            PlugManager *(*PlugManager__sharedInstance)();
            PlugWorkflowManager *(*PlugWorkflowManager_sharedInstance)();
            WirelessManager *(*WirelesManager_sharedInstance)();
            ConfirmationDialog *(*ConfirmationDialogFactory_getConfirmationDialog)(QWidget*);
            void (*ConfirmationDialog__setTitle)(ConfirmationDialog* _this, QString const&);
            void (*ConfirmationDialog__setText)(ConfirmationDialog* _this, QString const&);
            void (*ConfirmationDialog__setAcceptButtonText)(ConfirmationDialog* _this, QString const&);
            void (*ConfirmationDialog__setRejectButtonText)(ConfirmationDialog* _this, QString const&);
            MainWindowController *(*MainWindowController_sharedInstance)();
            void (*MainWindowController_toast)(MainWindowController*, QString const&, QString const&, int);
        } nSym;

        void ndbResolveSymbol(const char *name, void** sym);
        bool ndbInUSBMS();
        bool ndbActionStrValid(QString const& actStr);
        void ndbWireless(const char *act);
        void ndbSettings(QString const& action, const char* setting);
        void ndbNickelMisc(const char *action);
        QString getNickelMetaObjectDetails(const QMetaObject* nmo);
        template <typename T>
        void ndbConnectSignal(T *srcObj, const char *srcSignal, const char *dest);
        void dlgConfirmation(QString const& title, QString const& body, QString const& acceptText, QString const& rejectText);
        void pwrAction(const char *action);
};

#endif