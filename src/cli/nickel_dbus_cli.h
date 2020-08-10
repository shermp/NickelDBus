#ifndef NICKEL_DBUS_CLI_H
#define NICKEL_DBUS_CLI_H

#include <QObject>
#include "../interface/nickel_dbus_proxy.h"

class NDBCli : public QObject {
    Q_OBJECT

    public:
        NDBCli(QObject* parent, com::github::shermp::nickeldbus* ndb);

        void setMethodName(QString name);
        void setMethodArgs(QStringList args);
        void setSignalNames(QStringList names);
        void setTimeout(int timeout);
        void setPrintAPI(bool api);
    
    Q_SIGNALS:
        void timeoutTriggered();
    public Q_SLOTS:
        void start();
        void handleSignal();
        void handleSignal(int);
        void handleSignal(bool);
        void handleSignal(double);
        void handleSignal(QString);
        void handleTimeout();
    private:
        QString errString;
        QString methodName;
        QStringList methodArgs;
        QStringList signalNames;
        bool signalComplete, methodComplete, printApi;
        int timeout;
        com::github::shermp::nickeldbus* ndb;
        int callMethod();
        bool validateArgCount();
        int connectSignals();
        void processSignal(QString name, QString param = QString());
        void printMethods(int methodType);
        void printAPI();
};

#endif
