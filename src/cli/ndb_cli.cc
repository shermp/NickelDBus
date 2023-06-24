#include <QCoreApplication>
#include <QMetaObject>
#include <QMetaMethod>
#include <QMetaType>
#include <QTextStream>
#include <QStringList>
#include <QDebug>
#include <QTimer>

#include <type_traits>

#include "ndb_cli.h"

MethodParamList::MethodParamList() {
    mp.resize(10);
}

MethodParamList::~MethodParamList() {
    for (int i = 0; i < mp.size(); ++i) {
        if (mp[i].param) {
            QMetaType::destroy(mp[i].type, mp[i].param);
        }
    }
}

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

bool NDBCli::convertParam(int index, int typeID, void *param) {
    bool ok = false;

    if (typeID == QMetaType::Type::Int) {
        int *i = reinterpret_cast<int*> (param);
        *i = methodArgs.at(index).toInt(&ok); 

    } else if (typeID == QMetaType::Type::Bool) {
        bool *b = reinterpret_cast<bool*> (param);
        ok = true;
        if (!methodArgs.at(index).compare("true", Qt::CaseInsensitive) || !methodArgs.at(index).compare("t", Qt::CaseInsensitive)) {
            *b = true;
        } else if (!methodArgs.at(index).compare("false", Qt::CaseInsensitive) || !methodArgs.at(index).compare("f", Qt::CaseInsensitive)) {
            *b = false;
        } else {
            ok = false;
        }

    } else if (typeID == QMetaType::Type::QString) {
        QString *s = reinterpret_cast<QString*> (param);
        s->append(methodArgs.at(index));
        ok = true;

    } else {
        ok = false;
    }
    return ok;
}

template<typename T>
int NDBCli::printMethodReply(void *reply) {
    int rv = -1;
    QDBusPendingReply<T> *r = reinterpret_cast<QDBusPendingReply<T>*>(reply);
    r->waitForFinished();
    if (!r->isError()) {
        if (r->count() > 0) {
            QVariant qv = r->argumentAt(0);
            if (qv.type() == QVariant::Type::StringList) {
                QStringList sl = qv.toStringList();
                for (int i = 0; i < sl.size(); ++i) {
                    QTextStream(stdout) << sl.at(i) << endl;
                }
            } else {
                QTextStream(stdout) << qv.toString() << endl;
            }
        }
        rv = 0;
    } else {
        errString = QString("method failed with err: %1 and message: %2").arg(QDBusError::errorString(r->error().type())).arg(r->error().message());
    }
    return rv;
}

// We seem to need to make this a separate function from above, as the compiler
// throws a fit when typename parameter is void.
int NDBCli::printMethodReply(void *reply) {
    int rv = -0;
    QDBusPendingReply<> *r = reinterpret_cast<QDBusPendingReply<>*>(reply);
    r->waitForFinished();
    if (r->isError()) {
        errString = QString("method failed with err: %1 and message: %2").arg(QDBusError::errorString(r->error().type())).arg(r->error().message());
        rv = -1;
    }
    return rv;
}

int NDBCli::callMethodInvoke() {
    int methodIndex = getMethodIndex();
    if (methodIndex < 0) {
        errString = QStringLiteral("non-existent method or invalid parameter count");
        return -1;
    }
    QMetaMethod m = ndb->metaObject()->method(methodIndex);
    if (m.parameterCount() > 10) {
        errString = QStringLiteral("a maximum of 10 parameters are allowed");
        return -1;
    }
    MethodParamList params = MethodParamList();
    for (int i = 0; i < m.parameterCount(); ++i) {
        params.mp[i].type = m.parameterType(i);
        params.mp[i].param = QMetaType::create(params.mp[i].type);
        if (!convertParam(i, params.mp[i].type, params.mp[i].param)) {
            errString = QString("unable to convert parameter %1").arg(QString(m.parameterNames().at(i)));
            return -1;
        }
        params.mp[i].genericArg = QGenericArgument(m.parameterNames().at(i), params.mp[i].param);
    }
    // QDBusPendingReply<> doesn't appear to be a QObject, and even if it is, it
    // isn't registered as a QMetaType. We'll take care of it here, and register
    // one of every type we may currently encounter (plus int, because it could
    // be used in the future).
    int voidPR = qRegisterMetaType<QDBusPendingReply<>>("QDBusPendingReply<>");
    int strPR = qRegisterMetaType<QDBusPendingReply<QString>>("QDBusPendingReply<QString>");
    int strLstPR = qRegisterMetaType<QDBusPendingReply<QStringList>>("QDBusPendingReply<QStringList>");
    int boolPR = qRegisterMetaType<QDBusPendingReply<bool>>("QDBusPendingReply<bool>");
    int intPR = qRegisterMetaType<QDBusPendingReply<int>>("QDBusPendingReply<int>");
    int id = QMetaType::type(m.typeName());
    if (id == QMetaType::UnknownType) {
        errString = QStringLiteral("could not create variable of unknown type");
        return -1;
    }
    void *ret = QMetaType::create(id);
    if (!ret) {
        errString = QStringLiteral("unable to create return variable");
        return -1;
    }
    if (!m.invoke(ndb, 
        Qt::DirectConnection, 
        QGenericReturnArgument(m.typeName(), ret), 
        params.mp[0].genericArg, params.mp[1].genericArg, params.mp[2].genericArg, params.mp[3].genericArg, params.mp[4].genericArg,
        params.mp[5].genericArg, params.mp[6].genericArg, params.mp[7].genericArg, params.mp[8].genericArg, params.mp[9].genericArg
    )) {
        errString = QString("unable to call method %1").arg(QString(m.methodSignature()));
        QMetaType::destroy(id, ret);
        return -1;
    }
    // Stupid templated class. Is there a way of making the following more generic?
    int printRV;
    if      (id == voidPR) {printRV = printMethodReply(ret);}
    else if (id == strPR)  {printRV = printMethodReply<QString>(ret);}
    else if (id == boolPR) {printRV = printMethodReply<bool>(ret);}
    else if (id == intPR)  {printRV = printMethodReply<int>(ret);}
    else if (id == strLstPR)  {printRV = printMethodReply<QStringList>(ret);}
    else {printRV = -1;}
    QMetaType::destroy(id, ret);
    return printRV;
}

#define NDBCLI_SIG_NAME() QString(sender()->metaObject()->method(senderSignalIndex()).name())
#define NDBCLI_SIG_CONNECT(signal, handler) QObject::connect(ndb, &NDBProxy::signal, this, &NDBCli::handler)

void NDBCli::connectSignals() {
    NDBCLI_SIG_CONNECT(dlgConfirmResult, handleSignalParam1);
    NDBCLI_SIG_CONNECT(dlgConfirmTextInput, handleSignalParam1);
    NDBCLI_SIG_CONNECT(pfmAboutToConnect, handleSignalParam0);
    NDBCLI_SIG_CONNECT(pfmDoneProcessing, handleSignalParam0);
    NDBCLI_SIG_CONNECT(fssFinished, handleSignalParam0);
    NDBCLI_SIG_CONNECT(fssGotNumFilesToProcess, handleSignalParam1);
    NDBCLI_SIG_CONNECT(fssParseProgress, handleSignalParam1);
    NDBCLI_SIG_CONNECT(wmLinkQualityForConnectedNetwork, handleSignalParam1);
    NDBCLI_SIG_CONNECT(wmMacAddressAvailable, handleSignalParam1);
    NDBCLI_SIG_CONNECT(wmNetworkConnected, handleSignalParam0);
    NDBCLI_SIG_CONNECT(wmNetworkDisconnected, handleSignalParam0);
    NDBCLI_SIG_CONNECT(wmNetworkFailedToConnect, handleSignalParam0);
    NDBCLI_SIG_CONNECT(wmNetworkForgotten, handleSignalParam0);
    NDBCLI_SIG_CONNECT(wmScanningAborted, handleSignalParam0);
    NDBCLI_SIG_CONNECT(wmScanningFinished, handleSignalParam0);
    NDBCLI_SIG_CONNECT(wmScanningStarted, handleSignalParam0);
    NDBCLI_SIG_CONNECT(wmTryingToConnect, handleSignalParam0);
    NDBCLI_SIG_CONNECT(wmWifiEnabled, handleSignalParam1);
    NDBCLI_SIG_CONNECT(ndbViewChanged, handleSignalParam1);
    NDBCLI_SIG_CONNECT(rvPageChanged, handleSignalParam1);
}

void NDBCli::handleSignalParam0() {
    handleSignal(NDBCLI_SIG_NAME());
}

void NDBCli::handleSignalParam1(QVariant val1) {
    handleSignal(NDBCLI_SIG_NAME(), val1);
}

void NDBCli::handleSignalParam2(QVariant val1, QVariant val2) {
    handleSignal(NDBCLI_SIG_NAME(), val1, val2);
}

void NDBCli::handleSignal(const QString& sigName, QVariant val1, QVariant val2, QVariant val3, QVariant val4) {
    if (signalNames.contains(sigName)) {
        QTextStream out(stdout);
        out << sigName;
        if (val1.isValid()) { out << " " << val1.toString(); }
        if (val2.isValid()) { out << " " << val2.toString(); }
        if (val3.isValid()) { out << " " << val3.toString(); }
        if (val4.isValid()) { out << " " << val4.toString(); }
        out << endl;
        if (methodName.isEmpty() || methodComplete) {
        QCoreApplication::quit();
        } else {
            signalComplete = true;
        }
    }
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
        connectSignals();
    }
    if (!methodName.isEmpty()) {
        if (callMethodInvoke() != 0) {
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
