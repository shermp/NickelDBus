#include <dlfcn.h>
#include <QString>
#include "nickel_dbus.h"
#include "adapter/nickel_dbus_adapter.h"
#include "nm/util.h"

typedef QObject PlugWorkflowManager;

NickelDBus::NickelDBus(QObject* parent) : QObject(parent) {
    new NickelDBusAdapter(this);
    this->dbusRegSucceeded = true;
    QDBusConnection conn = QDBusConnection::systemBus();
    if (!conn.registerObject("/nickeldbus", this)) {
        NM_LOG("failed to register object on system bus");
        this->dbusRegSucceeded = false;
    }
    if (!conn.registerService("local.shermp.nickeldbus")) {
        NM_LOG("failed to register service on the system bus");
        this->dbusRegSucceeded = false;
    }
    this->libnickel = nullptr;
}
NickelDBus::~NickelDBus() {
    QDBusConnection conn = QDBusConnection::systemBus();
    conn.unregisterService("local.shermp.nickeldbus");
    conn.unregisterObject("/nickeldbus");
}

bool NickelDBus::connectSignals(char **err_out) {
    #define NM_ERR_RET false
    //libnickel 4.13.12638 * _ZN19PlugWorkflowManager14sharedInstanceEv
    PlugWorkflowManager *(*PlugWorkflowManager_sharedInstance)();
    reinterpret_cast<void*&>(PlugWorkflowManager_sharedInstance) = dlsym(this->libnickel, "_ZN19PlugWorkflowManager14sharedInstanceEv");
    NM_ASSERT(PlugWorkflowManager_sharedInstance, "could not dlsym PlugWorkflowManager::sharedInstance");

    PlugWorkflowManager *wf = PlugWorkflowManager_sharedInstance();
    NM_ASSERT(wf, "could not get shared PlugWorkflowManager pointer");
    NM_LOG("connecting PlugWorkflowManager::doneProcessing");
    QObject::connect(wf, SIGNAL(doneProcessing()), this, SIGNAL(pfmDoneProccessing()));
    NM_RETURN_OK(true);
    #undef NM_ERR_RET
}

QString NickelDBus::version() {
    return QString("NickelDBus-0.0.0");
}

bool NickelDBus::pfmRescanBooksFull() {
    return true;
}