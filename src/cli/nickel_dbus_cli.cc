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

int NDBCli::getMethodIndex() {
    const QMetaObject *mo = ndb->metaObject();
    for (int i = mo->methodOffset(); i < mo->methodCount(); ++i ) {
        QMetaMethod method = mo->method(i);
        if (!QString(method.name()).compare(methodName) && (method.methodType() == QMetaMethod::Method || method.methodType() == QMetaMethod::Slot)) {
            if (method.parameterCount() == methodArgs.size()) {
                return i;
            }
        }
    }
    return -1;
}

QVariantList NDBCli::convertParams(int methodIndex, bool *ok) {
    QVariantList args = QVariantList();
    QMetaMethod m = ndb->metaObject()->method(methodIndex);
    QList<QByteArray> paramNames = m.parameterNames();
    for (int i = 0; i < m.parameterCount(); ++i) {
        switch (m.parameterType(i)) {
        case QMetaType::Type::Int:
            args.append(methodArgs.at(i).toInt(ok)); 
            if (!*ok) {
                errString = QString("could not parse integer: %1").arg(QString(paramNames.at(i)));
            } 
            break;
        case QMetaType::Type::Bool:
            *ok = true;
            if (!methodArgs.at(i).compare("true", Qt::CaseInsensitive) || !methodArgs.at(i).compare("t", Qt::CaseInsensitive)) {
                args.append(true);
            } else if (!methodArgs.at(i).compare("false", Qt::CaseInsensitive) || !methodArgs.at(i).compare("f", Qt::CaseInsensitive)) {
                args.append(false);
            } else {
                errString =  QString("could not parse bool: %1. One of 'true', 't', 'false', 'f' required").arg(QString(paramNames.at(i)));
                *ok = false;
            }
            break;
        case QMetaType::Type::QString:
            args.append(methodArgs.at(i));
            *ok = true;
            break;
        default:
            errString =  QString("unsupported type: %1").arg(QString(paramNames.at(i)));
            *ok = false;
        }
        if (!*ok) {
            return args;
        }
    }
    return args;
}

int NDBCli::callMethod() {
    // Forgive the if-ladder, but it's the easiest way to do this... Note, this
    // can probably be done more consisely with QMetaObjects and QMetaMethods,
    // but dealing with the types there makes my head hurt...
    //
    // To elaborate, first we must obtain the QMetaMethod (easy enough). Then we
    // have to get the number of parameters for that method. Following that, one
    // has to convert the QString args from the command list into usable types,
    // perhaps into a QVariantList. Invoking the method involves setting the
    // parameters using Q_ARG() macros, which involve knowing the type of each
    // arg. And then there's the return value. That requires a pre-declared
    // variable, and it's type, and the invoke method stores the return value
    // there. Our return type happens to be a templated class
    // (QDBusPendingReply<T>).
    //
    // I've decided it's all in the too-hard basket.
    int methodIndex = getMethodIndex();
    if (methodIndex < 0) {
        errString = QStringLiteral("non-existent method or invalid parameter count");
        return -1;
    }
    bool args_ok;
    QVariantList args = convertParams(methodIndex, &args_ok);
    if (!args_ok) {
        // convertArgs already sets the error string
        return -1;
    }
    if (!methodName.compare("ndbVersion")) {
        NDBCLI_CALL_METHOD(QString, ndb->ndbVersion());
    } else if (!methodName.compare("miscNickelClassDetails")) {
        NDBCLI_CALL_METHOD(QString, ndb->miscNickelClassDetails(args.at(0).toString()));
    } else if (!methodName.compare("miscSignalConnected")) {
        NDBCLI_CALL_METHOD(bool, ndb->miscSignalConnected(args.at(0).toString()));
    } else if (!methodName.compare("mwcToast")) {
        if (methodArgs.size() == 2) {
            NDBCLI_CALL_METHOD_VOID(ndb->mwcToast(args.at(0).toInt(), args.at(1).toString()));
        } else {
            NDBCLI_CALL_METHOD_VOID(ndb->mwcToast(args.at(0).toInt(), args.at(1).toString(), args.at(2).toString()));
        }
    } else if (!methodName.compare("mwcHome")) {
        NDBCLI_CALL_METHOD_VOID(ndb->mwcHome());
    } else if (!methodName.compare("dlgConfirmNoBtn")) {
        NDBCLI_CALL_METHOD_VOID(ndb->dlgConfirmNoBtn(args.at(0).toString(), args.at(1).toString()));
    } else if (!methodName.compare("dlgConfirmAccept")) {
        NDBCLI_CALL_METHOD_VOID(ndb->dlgConfirmAccept(args.at(0).toString(), args.at(1).toString(), args.at(2).toString()));
    } else if (!methodName.compare("dlgConfirmReject")) {
        NDBCLI_CALL_METHOD_VOID(ndb->dlgConfirmReject(args.at(0).toString(), args.at(1).toString(), args.at(2).toString()));
    } else if (!methodName.compare("dlgConfirmAcceptReject")) {
        NDBCLI_CALL_METHOD_VOID(ndb->dlgConfirmAcceptReject(args.at(0).toString(), args.at(1).toString(), args.at(2).toString(), args.at(3).toString()));
    } else if (!methodName.compare("pfmRescanBooks")) {
        NDBCLI_CALL_METHOD_VOID(ndb->pfmRescanBooks());
    } else if (!methodName.compare("pfmRescanBooksFull")) {
        NDBCLI_CALL_METHOD_VOID(ndb->pfmRescanBooksFull());
    } else if (!methodName.compare("wfmConnectWireless")) {
        NDBCLI_CALL_METHOD_VOID(ndb->wfmConnectWireless());
    } else if (!methodName.compare("wfmConnectWirelessSilently")) {
        NDBCLI_CALL_METHOD_VOID(ndb->wfmConnectWirelessSilently());
    } else if (!methodName.compare("wfmSetAirplaneMode")) {
        NDBCLI_CALL_METHOD_VOID(ndb->wfmSetAirplaneMode(args.at(0).toString()));
    } else if (!methodName.compare("bwmOpenBrowser")) {
        if (methodArgs.count() > 0) {
            if (methodArgs.count() == 1) {
                NDBCLI_CALL_METHOD_VOID(ndb->bwmOpenBrowser(args.at(0).toBool()));
            } else if (methodArgs.count() == 2) {
                NDBCLI_CALL_METHOD_VOID(ndb->bwmOpenBrowser(args.at(0).toBool(), args.at(1).toString()));
            } else {
                NDBCLI_CALL_METHOD_VOID(ndb->bwmOpenBrowser(args.at(0).toBool(), args.at(1).toString(), args.at(2).toString()));
            }
        }
        NDBCLI_CALL_METHOD_VOID(ndb->bwmOpenBrowser());
    } else if (!methodName.compare("nsInvert")) {
        NDBCLI_CALL_METHOD_VOID(ndb->nsInvert(args.at(0).toString()));
    } else if (!methodName.compare("nsLockscreen")) {
        NDBCLI_CALL_METHOD_VOID(ndb->nsLockscreen(args.at(0).toString()));
    } else if (!methodName.compare("nsScreenshots")) {
        NDBCLI_CALL_METHOD_VOID(ndb->nsScreenshots(args.at(0).toString()));
    } else if (!methodName.compare("nsForceWifi")) {
        NDBCLI_CALL_METHOD_VOID(ndb->nsForceWifi(args.at(0).toString()));
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
                } else {
                    errString = QString("cannot handle signal with more than one parameter");
                    return -1;
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
    qCritical() << "timeout expired after" << timeout << "milliseconds";
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

void NDBCli::setPrintAPI(bool api) {
    printApi = api;
}

void NDBCli::printMethods(int methodType) {
    QTextStream methodOut(stdout);
    const QMetaObject *mo = ndb->metaObject();
    for (int i = mo->methodOffset(); i < mo->methodCount(); ++ i) {
        QMetaMethod method = mo->method(i);
        if (method.methodType() == methodType) {
            methodOut << "    " << method.name();
            auto params = method.parameterNames();
            auto paramTypes = method.parameterTypes();
            for (int j = 0; j < params.size(); ++j) {
                methodOut << " " << "<" << paramTypes.at(j) << ">" << " " << params.at(j);
                if (j < params.size() - 1) {
                    methodOut << ",";
                }
            }
            methodOut << endl;
        }
    }
}

void NDBCli::printAPI() {
    QTextStream(stdout) << "The following methods and their arguments can be called:" << endl;
    printMethods(QMetaMethod::Slot);
    QTextStream(stdout) << "\nThe following signals and their 'return' value can be waited for:" << endl;
    printMethods(QMetaMethod::Signal);
}

void NDBCli::start() {
    if (!ndb->isValid()) {
        qCritical() << "interface not valid";
        QCoreApplication::exit(1);
    }
    if (printApi) {
        printAPI();
        QCoreApplication::quit();
    }
    if (signalNames.size() > 0) {
        if (connectSignals() != 0) {
            qCritical() << "failed with: " << errString;
            QCoreApplication::exit(1);
        }
    }
    if (!methodName.isEmpty()) {
        if (callMethod() != 0) {
            qCritical() << "failed with: " << errString;
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
