#ifndef NICKEL_DBUS_H
#define NICKEL_DBUS_H
#include <QObject>
#include <QString>
#include <QSet>
#include <QtDBus>
#include <QDBusContext>

typedef void PlugManager;
typedef QObject PlugWorkflowManager;
typedef QObject WirelessManager;
typedef void MainWindowController;

#ifndef NDB_DBUS_IFACE_NAME
    #define NDB_DBUS_IFACE_NAME "local.shermp.nickeldbus"
#endif
#ifndef NDB_DBUS_OBJECT_PATH
    #define NDB_DBUS_OBJECT_PATH "/nickeldbus"
#endif
class NickelDBus : public QObject, protected QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", NDB_DBUS_IFACE_NAME)

    QDBusConnection conn = QDBusConnection::systemBus();
    public:
        void *libnickel;
        bool initSucceeded;
        NickelDBus(QObject* parent);
        ~NickelDBus();
        // bool registerDBus();
        void connectSignals();
        bool testAssert(bool test);
    Q_SIGNALS:
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

    public Q_SLOTS:
        QString version();
        QString nickelClassDetails(QString const& static_metaobject_symbol);
        // misc
        bool signalConnected(QString const& signal_name);
        int showToast(int toast_duration, QString const& msg_main, QString const& msg_sub = QStringLiteral(""));
        int goHome();
        // PlugworkFlowManager
        int pfmRescanBooks();
        int pfmRescanBooksFull();
        // Wireless methods (WirelessFlowManager)
        int wfmConnectWireless();
        int wfmConnectWirelessSilently();
        int wfmSetAirplaneMode(QString const& action);
        // Web Browser (BrowserWorkflowManager)
        int bwmOpenBrowser(bool modal = false, QString const& url = QString(), QString const& css = QString());
        // Nickel Settings
        int nsInvert(QString const& action);
        int nsLockscreen(QString const& action);
        int nsScreenshots(QString const& action);
        int nsForceWifi(QString const& action);
    private:
        QSet<QString> connectedSignals;
        bool *(*PlugManager__gadgetMode)(PlugManager*);
        PlugManager *(*PlugManager__sharedInstance)();
        bool ndbInUSBMS();
        bool ndbActionStrValid(QString const& actStr);
        int ndbWireless(const char *act);
        int ndbSettings(QString const& action, const char* setting);
        int ndbNickelMisc(const char *action);

        template <typename T>
        void ndbConnectSignal(T *srcObj, const char *srcSignal, const char *dest);
};

#endif