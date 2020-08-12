#include <QCoreApplication>
#include <QMetaObject>
#include <QMetaMethod>
#include <QMetaType>
#include <QTextStream>
#include <QDebug>
#include <QTimer>

#include <type_traits>

#include "nickel_dbus_cli.h"

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
            QTextStream(stdout) << r->value() << endl;
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
    QList<void*> params = QList<void*>();
    QList<QGenericArgument> genericArgs = QList<QGenericArgument>();
    for (int i = 0; i < m.parameterCount(); ++i) {
        params.append(QMetaType::create(m.parameterType(i)));
        if (!convertParam(i, m.parameterType(i), params.at(i))) {
            errString = QString("unable to convert parameter %1").arg(QString(m.parameterNames().at(i)));
            return -1;
        }
        genericArgs.append(QGenericArgument(m.parameterNames().at(i), params.at(i)));
    }
    for (int i = m.parameterCount(); i < 10; ++i) {
        genericArgs.append(QGenericArgument());
    }
    // QDBusPendingReply<> doesn't appear to be a QObject, and even if it is, it
    // isn't registered as a QMetaType. We'll take care of it here, and register
    // one of every type we may currently encounter (plus int, because it could
    // be used in the future).
    int voidPR = qRegisterMetaType<QDBusPendingReply<>>("QDBusPendingReply<>");
    int strPR = qRegisterMetaType<QDBusPendingReply<QString>>("QDBusPendingReply<QString>");
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
        genericArgs.at(0), genericArgs.at(1), genericArgs.at(2), genericArgs.at(3), genericArgs.at(4),
        genericArgs.at(5), genericArgs.at(6), genericArgs.at(7), genericArgs.at(8), genericArgs.at(9)
    )) {
        errString = QString("unable to call method %1").arg(QString(m.methodSignature()));
        return -1;
    }
    // Stupid templated class. Is there a way of making the following more generic?
    int printRV;
    if      (id == voidPR) {printRV = printMethodReply(ret);}
    else if (id == strPR)  {printRV = printMethodReply<QString>(ret);}
    else if (id == boolPR) {printRV = printMethodReply<bool>(ret);}
    else if (id == intPR)  {printRV = printMethodReply<int>(ret);}
    else {printRV = -1;}
    return printRV;
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
