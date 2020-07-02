#ifndef NICKEL_DBUS_H
#define NICKEL_DBUS_H
#include <QObject>
#include <QString>
#include <QSet>
#include <QtDBus>

typedef enum ndb_err {
    ndb_err_ok = 0,
    ndb_err_inval_param = 1,
    ndb_err_dlsym = 2,
    ndb_err_call = 3,
    ndb_err_usb = 4
} ndb_err;

class NickelDBus : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "local.shermp.nickeldbus")
    public:
        void *libnickel;
        bool dbusRegSucceeded;
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
        // PlugworkFlowManager
        int pfmRescanBooksFull();
        // Wireless methods (WirelessFlowManager)
        int wfmConnectWireless();
        int wfmConnectWirelessSilently();
        int wfmSetAirplaneMode(bool enabled);
        // Web Browser (BrowserWorkflowManager)
        int bwmOpenBrowser(bool modal = false, QString const& url = QString(), QString const& css = QString());
    protected Q_SLOTS:
        void enableMethodInhibit();
        void disableMethodInhibit();
    private:
        enum wireless_conn_option {AUTO, AUTO_SILENT, ENABLE, DISABLE};
        bool methodsInhibited;
        QSet<QString> connectedSignals;

        int ndbWireless(enum wireless_conn_option);
};

#endif