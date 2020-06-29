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
    ndb_err_call = 3
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
        void pfmDoneProccessing(bool done = true);
    
    public Q_SLOTS:
        QString version();
        bool signalConnected(QString const &signal_name);
        int showToast(int toast_duration, QString const &msg_main, QString const &msg_sub = QStringLiteral(""));
        int pfmRescanBooksFull();
    private:
        QSet<QString> connectedSignals;
};

#endif