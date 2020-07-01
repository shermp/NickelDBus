#include <dlfcn.h>
#include <QString>
#include <unistd.h>
#include "../NickelMenu/src/action.h"
#include "util.h"
#include "nickel_dbus.h"
#include "adapter/nickel_dbus_adapter.h"

typedef QObject PlugWorkflowManager;
typedef void MainWindowController;

NickelDBus::NickelDBus(QObject* parent) : QObject(parent) {
    new NickelDBusAdapter(this);
    this->dbusRegSucceeded = true;
    QDBusConnection conn = QDBusConnection::systemBus();
    if (!conn.registerObject("/nickeldbus", this)) {
        NDB_LOG("failed to register object on system bus");
        this->dbusRegSucceeded = false;
    }
    if (!conn.registerService("local.shermp.nickeldbus")) {
        NDB_LOG("failed to register service on the system bus");
        this->dbusRegSucceeded = false;
    }
    this->libnickel = nullptr;
}
NickelDBus::~NickelDBus() {
    QDBusConnection conn = QDBusConnection::systemBus();
    conn.unregisterService("local.shermp.nickeldbus");
    conn.unregisterObject("/nickeldbus");
}

void NickelDBus::connectSignals() {
    PlugWorkflowManager *(*PlugWorkflowManager_sharedInstance)();
    reinterpret_cast<void*&>(PlugWorkflowManager_sharedInstance) = dlsym(this->libnickel, "_ZN19PlugWorkflowManager14sharedInstanceEv");
    if (PlugWorkflowManager_sharedInstance) {
        PlugWorkflowManager *wf = PlugWorkflowManager_sharedInstance();
        if (wf) {
            NDB_LOG("connecting PlugWorkflowManager::doneProcessing");
            if (QObject::connect(wf, SIGNAL(doneProcessing()), this, SIGNAL(pfmDoneProccessing()))) {
                connectedSignals.insert("pfmDoneProccessing");
            } else {NDB_LOG("PlugWorkflowManager::doneProcessing connection failed");}
        } else {NDB_LOG("could not get shared PlugWorkflowManager pointer");}
    } else {NDB_LOG("could not dlsym PlugWorkflowManager::sharedInstance");}
}

QString NickelDBus::version() {
    return QString(NDB_VERSION);
}
QString NickelDBus::nickelClassDetails(QString const& static_metaobject_symbol) {
    typedef QMetaObject NickelMetaObject;
    NDB_ASSERT(QString("ERROR: not a valid staticMetaObject symbol"), static_metaobject_symbol.endsWith(QString("staticMetaObjectE")), "not a valid staticMetaObject symbol");
    QByteArray sym = static_metaobject_symbol.toLatin1();
    NickelMetaObject *nmo;
    reinterpret_cast<void*&>(nmo) = dlsym(this->libnickel, sym.constData());
    NDB_ASSERT(QString("ERROR: DLSYM"), nmo, "could not dlsym staticMetaObject function for symbol %s", sym.constData());
    QString str = QString("");
    str.append(QString("Showing meta information for Nickel class %1 : \n").arg(nmo->className()));
    str.append("Properties : \n");
    for (int i = nmo->propertyOffset(); i < nmo->propertyCount(); ++i) {
        QMetaProperty prop = nmo->property(i);
        str.append(QString("\t%1 %2 :: readable: %3 :: writeable: %4\n").arg(prop.typeName()).arg(prop.name()).arg(prop.isReadable()).arg(prop.isWritable())); 
    }
    str.append("Methods : \n");
    for (int i = nmo->methodOffset(); i < nmo->methodCount(); ++i) {
        QMetaMethod method = nmo->method(i);
        const char *method_type;
        switch (method.methodType()) {
            case QMetaMethod::Signal:
                method_type = "SIGNAL";
                break;
            case QMetaMethod::Slot:
                method_type = "SLOT";
                break;
            case QMetaMethod::Method:
                method_type = "METHOD";
                break;
            case QMetaMethod::Constructor:
                method_type = "CONSTRUCTOR";
                break;
            default:
                method_type = "UNKOWN";
        }
        str.append(QString("\t%1 :: %2 %3\n").arg(method_type).arg(method.typeName()).arg(method.methodSignature().constData()));
    }
    return str;
}
bool NickelDBus::testAssert(bool test) {
    NDB_ASSERT(false, test, "The test value was '%s'", (test ? "true" : "false"));
    return true;
}

bool NickelDBus::signalConnected(QString const &signal_name) {
    return connectedSignals.contains(signal_name);
}

int NickelDBus::showToast(int toast_duration, QString const &msg_main, QString const &msg_sub) {
    // The following code has been adapted from NickelMenu
    NDB_ASSERT(ndb_err_inval_param, toast_duration > 0 && toast_duration <= 5000, "toast duration must be between 0 and 5000 miliseconds");
    MainWindowController *(*MainWindowController_sharedInstance)();
    void (*MainWindowController_toast)(MainWindowController*, QString const&, QString const&, int);
    //libnickel 4.6 * _ZN20MainWindowController14sharedInstanceEv
    reinterpret_cast<void*&>(MainWindowController_sharedInstance) = dlsym(libnickel, "_ZN20MainWindowController14sharedInstanceEv");
    NDB_ASSERT(ndb_err_dlsym, MainWindowController_sharedInstance, "unsupported firmware: could not find MainWindowController::sharedInstance()");
    //libnickel 4.6 * _ZN20MainWindowController5toastERK7QStringS2_i
    reinterpret_cast<void*&>(MainWindowController_toast) = dlsym(libnickel, "_ZN20MainWindowController5toastERK7QStringS2_i");
    NDB_ASSERT(ndb_err_dlsym, MainWindowController_toast, "unsupported firmware: could not find MainWindowController::toast(QString const&, QString const&, int)");
    MainWindowController *mwc = MainWindowController_sharedInstance();
    NDB_ASSERT(ndb_err_call, mwc, "could not get MainWindowController instance");
    MainWindowController_toast(mwc, msg_main, msg_sub, toast_duration);
    return ndb_err_ok;
}

int NickelDBus::pfmRescanBooksFull() {
    char *err = NULL;
    nm_action_result_t *res = nm_action_nickel_misc("rescan_books_full", &err);
    if (!res) {
        free(err);
        return ndb_err_call;
    }
    nm_action_result_free(res);
    return ndb_err_ok;
}