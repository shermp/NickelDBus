#include <QCoreApplication>
#include <QMetaObject>
#include <QMetaMethod>
#include <QMetaType>
#include <QTextStream>
#include <QDebug>
#include <QTimer>

#include "nickel_dbus_cli.h"

#define NDBCLI_CALL_METHOD(retType, method) QDBusPendingReply<retType> r = method;          \
r.waitForFinished();                                                                        \
if (!r.isError()) {                                                                         \
    if (r.count() > 0) {QTextStream(stdout) << r.value() << endl;}                          \
    return 0;                                                                               \
} else {                                                                                    \
    errString = QString("method failed with err: %1 and message: %2").arg(QDBusError::errorString(r.error().type())).arg(r.error().message()); \
    return -1;                                                                               \
}
#define NDBCLI_CALL_METHOD_VOID(method) QDBusPendingReply<> r = method;                     \
r.waitForFinished();                                                                        \
if (!r.isError()) {                                                                         \
    return 0;                                                                               \
} else {                                                                                    \
    errString = QString("method failed with err: %1 and message: %2").arg(QDBusError::errorString(r.error().type())).arg(r.error().message()); \
    return -1;                                                                               \
}

#define NDBCLI_HANDLE_SIG_BODY() QMetaMethod method = sender()->metaObject()->method(senderSignalIndex()); \
    processSignal(QString(method.name()), QString(" %1").arg(val));

#define NDBCLI_CONNECT_SIGNAL(type) if (!QObject::connect(ndb, "2" + method.methodSignature(), this, SLOT(handleSignal(type)))) { \
    errString = QString("unable to connect %1 to handleSignals()").arg(QString(method.methodSignature()));                    \
    return -1;}

NDBCli::NDBCli(QObject* parent, com::github::shermp::nickeldbus *ndb) : QObject(parent) {
    this->ndb = ndb;
    signalComplete = methodComplete = false;
    methodName = QString();
    methodArgs = QStringList();
    signalNames = QStringList();
    timeout = -1;
}

bool NDBCli::validateArgCount() {
    bool retval = false;
    const QMetaObject *mo = ndb->metaObject();
    for (int i = mo->methodOffset(); i < mo->methodCount(); ++i ) {
        QMetaMethod method = mo->method(i);
        if (!QString(method.name()).compare(methodName) && (method.methodType() == QMetaMethod::Method || method.methodType() == QMetaMethod::Slot)) {
            if (method.parameterCount() == methodArgs.size()) {
                retval = true;
                break;
            }
        }
    }
    return retval;
}

int NDBCli::callMethod() {
    // Forgive the if-ladder, but it's the easiest way to do this...
    // Note, this can probably be done more consisely with QMetaObjects
    // and QMetaMethods, but dealing with the types there makes my head hurt...
    if (!validateArgCount()) {
        errString = QStringLiteral("invalid parameter count");
        return -1;
    }
    if (!methodName.compare("ndbVersion")) {
        NDBCLI_CALL_METHOD(QString, ndb->ndbVersion());
    } else if (!methodName.compare("miscNickelClassDetails")) {
        NDBCLI_CALL_METHOD(QString, ndb->miscNickelClassDetails(methodArgs.at(0)));
    } else if (!methodName.compare("miscSignalConnected")) {
        NDBCLI_CALL_METHOD(bool, ndb->miscSignalConnected(methodArgs.at(0)));
    } else if (!methodName.compare("mwcToast")) {
        bool conv_ok;
        int duration = methodArgs.at(0).toInt(&conv_ok);
        if (!conv_ok) {
            errString = QStringLiteral("could not parse timestamp");
            return -1;
        }
        if (methodArgs.size() == 2) {
            NDBCLI_CALL_METHOD_VOID(ndb->mwcToast(duration, methodArgs.at(1)));
        } else {
            NDBCLI_CALL_METHOD_VOID(ndb->mwcToast(duration, methodArgs.at(1), methodArgs.at(2)));
        }
    } else if (!methodName.compare("mwcHome")) {
        NDBCLI_CALL_METHOD_VOID(ndb->mwcHome());
    } else if (!methodName.compare("dlgConfirmNoBtn")) {
        NDBCLI_CALL_METHOD_VOID(ndb->dlgConfirmNoBtn(methodArgs.at(0), methodArgs.at(1)));
    } else if (!methodName.compare("dlgConfirmAccept")) {
        NDBCLI_CALL_METHOD_VOID(ndb->dlgConfirmAccept(methodArgs.at(0), methodArgs.at(1), methodArgs.at(2)));
    } else if (!methodName.compare("dlgConfirmReject")) {
        NDBCLI_CALL_METHOD_VOID(ndb->dlgConfirmReject(methodArgs.at(0), methodArgs.at(1), methodArgs.at(2)));
    } else if (!methodName.compare("dlgConfirmAcceptReject")) {
        NDBCLI_CALL_METHOD_VOID(ndb->dlgConfirmAcceptReject(methodArgs.at(0), methodArgs.at(1), methodArgs.at(2), methodArgs.at(3)));
    } else if (!methodName.compare("pfmRescanBooks")) {
        NDBCLI_CALL_METHOD_VOID(ndb->pfmRescanBooks());
    } else if (!methodName.compare("pfmRescanBooksFull")) {
        NDBCLI_CALL_METHOD_VOID(ndb->pfmRescanBooksFull());
    } else if (!methodName.compare("wfmConnectWireless")) {
        NDBCLI_CALL_METHOD_VOID(ndb->wfmConnectWireless());
    } else if (!methodName.compare("wfmConnectWirelessSilently")) {
        NDBCLI_CALL_METHOD_VOID(ndb->wfmConnectWirelessSilently());
    } else if (!methodName.compare("wfmSetAirplaneMode")) {
        NDBCLI_CALL_METHOD_VOID(ndb->wfmSetAirplaneMode(methodArgs.at(0)));
    } else if (!methodName.compare("bwmOpenBrowser")) {
        bool modal;
        if (methodArgs.count() > 0) {
            if (!methodArgs.at(0).compare("true", Qt::CaseInsensitive) || !methodArgs.at(0).compare("t", Qt::CaseInsensitive)) {
                modal = true;
            } else if (!methodArgs.at(0).compare("false", Qt::CaseInsensitive) || !methodArgs.at(0).compare("f", Qt::CaseInsensitive)) {
                modal = false;
            } else {
                errString =  QStringLiteral("could not parse modal. One of 'true', 't', 'false', 'f' required");
                return -1;
            }
            if (methodArgs.count() == 1) {
                NDBCLI_CALL_METHOD_VOID(ndb->bwmOpenBrowser(modal));
            } else if (methodArgs.count() == 2) {
                NDBCLI_CALL_METHOD_VOID(ndb->bwmOpenBrowser(modal, methodArgs.at(1)));
            } else {
                NDBCLI_CALL_METHOD_VOID(ndb->bwmOpenBrowser(modal, methodArgs.at(1), methodArgs.at(2)));
            }
        }
        NDBCLI_CALL_METHOD_VOID(ndb->bwmOpenBrowser());
    } else if (!methodName.compare("nsInvert")) {
        NDBCLI_CALL_METHOD_VOID(ndb->nsInvert(methodArgs.at(0)));
    } else if (!methodName.compare("nsLockscreen")) {
        NDBCLI_CALL_METHOD_VOID(ndb->nsLockscreen(methodArgs.at(0)));
    } else if (!methodName.compare("nsScreenshots")) {
        NDBCLI_CALL_METHOD_VOID(ndb->nsScreenshots(methodArgs.at(0)));
    } else if (!methodName.compare("nsForceWifi")) {
        NDBCLI_CALL_METHOD_VOID(ndb->nsForceWifi(methodArgs.at(0)));
    } else if (!methodName.compare("pwrShutdown")) {
        NDBCLI_CALL_METHOD_VOID(ndb->pwrShutdown());
    } else if (!methodName.compare("pwrReboot")) {
        NDBCLI_CALL_METHOD_VOID(ndb->pwrReboot());
    } 
    errString = QStringLiteral("unknown method");
    return -1;
}

int NDBCli::connectSignals() {
    const QMetaObject *mo = ndb->metaObject();
    for (int i = 0; i < signalNames.size(); ++i) {
        for (int j = mo->methodOffset(); j < mo->methodCount(); ++j ) {
            QMetaMethod method = mo->method(j);
            if (!QString(method.name()).compare(signalNames.at(i)) && method.methodType() == QMetaMethod::Signal) {
                if (method.parameterCount() == 0) {
                    NDBCLI_CONNECT_SIGNAL();
                } else if (method.parameterCount() == 1) {
                    switch (method.parameterType(0)) {
                    case QMetaType::Type::Int:
                        NDBCLI_CONNECT_SIGNAL(int); break;
                    case QMetaType::Type::Bool:
                        NDBCLI_CONNECT_SIGNAL(bool); break;
                    case QMetaType::Type::Double:
                        NDBCLI_CONNECT_SIGNAL(double); break;
                    case QMetaType::Type::QString:
                        NDBCLI_CONNECT_SIGNAL(QString); break;
                    default:
                        errString = QString("cannot handle signal with type %1").arg(QMetaType::typeName(method.parameterType(0)));
                        return -1;
                    }
                } 
            }
        }
    }
    return 0;
}

void NDBCli::processSignal(QString name, QString param) {
    QTextStream(stdout) << name << param << endl;
    if (methodName.isEmpty() || methodComplete) {
        QCoreApplication::quit();
    } else {
        signalComplete = true;
    }
}

void NDBCli::handleSignal() {
    QMetaMethod method = sender()->metaObject()->method(senderSignalIndex());
    processSignal(QString(method.name()));
}

void NDBCli::handleSignal(int val) {
    NDBCLI_HANDLE_SIG_BODY();
}

void NDBCli::handleSignal(bool val) {
    NDBCLI_HANDLE_SIG_BODY();
}

void NDBCli::handleSignal(double val) {
    NDBCLI_HANDLE_SIG_BODY();
}

void NDBCli::handleSignal(QString val) {
    NDBCLI_HANDLE_SIG_BODY();
}

void NDBCli::handleTimeout() {
    qCritical() << "timeout expired after" << timeout << "milliseconds" << endl;
    QCoreApplication::exit(1);
}

void NDBCli::setMethodName(QString name) {
    methodName = name;
}

void NDBCli::setMethodArgs(QStringList args) {
    methodArgs = args;
}

void NDBCli::setSignalNames(QStringList names) {
    signalNames = names;
}

void NDBCli::setTimeout(int t) {
    timeout = t;
}

void NDBCli::start() {
    if (!ndb->isValid()) {
        qCritical() << "interface not valid" << endl;
        QCoreApplication::exit(1);
    }
    if (signalNames.size() > 0) {
        if (connectSignals() != 0) {
            qCritical() << "failed with: " << errString << endl;
            QCoreApplication::exit(1);
        }
    }
    if (!methodName.isEmpty()) {
        if (callMethod() != 0) {
            qCritical() << "failed with: " << errString << endl;
            QCoreApplication::exit(1);
        } else {
            if (signalNames.size() > 0 && signalComplete) {
                QCoreApplication::quit();
            } else if (signalNames.size() == 0) {
                QCoreApplication::quit();
            } else {
                methodComplete = true;
            }
        }
    }
    if (timeout > 0) {
        QTimer::singleShot(timeout, this, SLOT(handleTimeout()));
    }
}
