#ifndef NICKEL_DBUS_CLI_H
#define NICKEL_DBUS_CLI_H

#include <QObject>
#include <QVector>
#include "../interface/ndb_proxy.h"

struct MethodParamList {
    struct MethodParam {
        int type;
        void *param;
        QGenericArgument genericArg;
    };
    QVector<MethodParam> mp;
    MethodParamList();
    ~MethodParamList();
};

class NDBCli : public QObject {
    Q_OBJECT

    public:
        NDBCli(QObject* parent, com::github::shermp::nickeldbus* ndb);

        void setMethodName(QString name);
        void setMethodArgs(QStringList args);
        void setSignalNames(QStringList names);
        void setTimeout(int timeout);
        void setPrintAPI(bool api);
        void handleSignalParam0();
        void handleSignalParam1(QVariant val1);
    Q_SIGNALS:
        void timeoutTriggered();
    public Q_SLOTS:
        void start();
        
        void handleTimeout();
    private:
        QString errString;
        QString methodName;
        QStringList methodArgs;
        QStringList signalNames;
        bool signalComplete, methodComplete, printApi;
        int timeout;
        com::github::shermp::nickeldbus* ndb;
        int callMethodInvoke();
        template<typename T>
        int printMethodReply(void *reply);
        int printMethodReply(void *reply);
        bool convertParam(int index, int typeID, void *param);
        int getMethodIndex();
        void connectSignals();
        void printMethods(int methodType);
        void printAPI();
        void handleSignal(const QString& sigName, QVariant = QVariant(), QVariant = QVariant(), QVariant = QVariant(), QVariant = QVariant());
};

#endif
