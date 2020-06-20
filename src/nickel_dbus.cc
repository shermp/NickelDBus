#include <dlfcn.h>
#include <QString>
#include <unistd.h>
#include "nickel_dbus.h"
#include "adapter/nickel_dbus_adapter.h"
#include "util.h"

typedef QObject PlugWorkflowManager;

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
            QObject::connect(wf, SIGNAL(doneProcessing()), this, SIGNAL(pfmDoneProccessing()));
        } else {NDB_LOG("could not get shared PlugWorkflowManager pointer");}
    } else {NDB_LOG("could not dlsym PlugWorkflowManager::sharedInstance");}
}

QString NickelDBus::version() {
    return QString("NickelDBus-0.0.0");
}
bool NickelDBus::testAssert(bool test) {
    NDB_ASSERT(false, false, test, "The test value was '%s'", (test ? "true" : "false"));
    return true;
}

bool NickelDBus::pfmRescanBooksFull() {
    // The following code is more-or-less lifted straight from NickelMenu

    //libnickel 4.13.12638 * _ZN19PlugWorkflowManager14sharedInstanceEv
    PlugWorkflowManager *(*PlugWorkflowManager_sharedInstance)();
    reinterpret_cast<void*&>(PlugWorkflowManager_sharedInstance) = dlsym(RTLD_DEFAULT, "_ZN19PlugWorkflowManager14sharedInstanceEv");
    NDB_ASSERT(false, nullptr, PlugWorkflowManager_sharedInstance, "could not dlsym PlugWorkflowManager::sharedInstance");

    // this is what is called by PlugWorkflowManager::plugged after confirmation
    //libnickel 4.13.12638 * _ZN19PlugWorkflowManager18onCancelAndConnectEv
    void (*PlugWorkflowManager_onCancelAndConnect)(PlugWorkflowManager*);
    reinterpret_cast<void*&>(PlugWorkflowManager_onCancelAndConnect) = dlsym(RTLD_DEFAULT, "_ZN19PlugWorkflowManager18onCancelAndConnectEv");
    NDB_ASSERT(false, nullptr, PlugWorkflowManager_onCancelAndConnect, "could not dlsym PlugWorkflowManager::onCancelAndConnect");

    //libnickel 4.13.12638 * _ZN19PlugWorkflowManager9unpluggedEv
    void (*PlugWorkflowManager_unplugged)(PlugWorkflowManager*);
    reinterpret_cast<void*&>(PlugWorkflowManager_unplugged) = dlsym(RTLD_DEFAULT, "_ZN19PlugWorkflowManager9unpluggedEv");
    NDB_ASSERT(false, nullptr, PlugWorkflowManager_unplugged, "could not dlsym PlugWorkflowManager::unplugged");

    PlugWorkflowManager *wf = PlugWorkflowManager_sharedInstance();
    NDB_ASSERT(false, nullptr, wf, "could not get shared PlugWorkflowManager pointer");

    PlugWorkflowManager_onCancelAndConnect(wf);
    sleep(1);
    PlugWorkflowManager_unplugged(wf);
    return true;
}