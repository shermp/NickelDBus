#include <QString>
#include <NickelHook.h>
#include "NDBVolContent.h"
#include "util.h"
#include "ndb.h"

namespace NDB {
NDBVolContent::NDBVolContent(QObject* parent) : QObject(parent) {
    initResult = Ok;
    resolveSymbolRTLD("_ZN13VolumeManager14sharedInstanceEv", nh_symoutptr(symbols.VolumeManager__sharedInstance));
    if (!symbols.VolumeManager__sharedInstance) {
        initResult = SymbolError;
        return;
    }
    Device *(*Device__getCurrentDevice)();
    resolveSymbolRTLD("_ZN6Device16getCurrentDeviceEv", nh_symoutptr(Device__getCurrentDevice));
    if (!Device__getCurrentDevice) {
        initResult = SymbolError;
        return;
    }
    Device *device = Device__getCurrentDevice();
    if (!device) {
        nh_log("could not get current device");
        initResult = NullError;
        return;
    }
    QString (*Device__getDbName)(Device* _this);
    resolveSymbolRTLD("_ZNK6Device9getDbNameEv", nh_symoutptr(Device__getDbName));
    if (!Device__getDbName) {
        initResult = SymbolError;
        return;
    }
    dbName = Device__getDbName(device);
    resolveSymbolRTLD("_ZN13VolumeManager7getByIdERK7QStringS2_", nh_symoutptr(symbols.VolumeManager__getById));
    resolveSymbolRTLD("_ZNK6Volume7isValidEv", nh_symoutptr(symbols.Volume__isValid));
    resolveSymbolRTLD("_ZNK7Content5getIdEv", nh_symoutptr(symbols.Content__getID));
    resolveSymbolRTLD("_ZNK7Content11getDbValuesEv", nh_symoutptr(symbols.Content__getDbValues));
    resolveSymbolRTLD("_ZNK6Volume11getDbValuesEv", nh_symoutptr(symbols.Volume__getDbValues));
    resolveSymbolRTLD("_ZN13VolumeManager7forEachERK7QStringRKSt8functionIFvRK6VolumeEE", nh_symoutptr(symbols.Volume__forEach));
    if (!symbols.VolumeManager__getById || 
        !symbols.Content__getID || 
        !symbols.Content__getDbValues ||
        !symbols.Volume__getDbValues ||
        !symbols.Volume__isValid ||
        !symbols.Volume__forEach) {
        initResult = SymbolError;
        return;
    }
}

NDBVolContent::~NDBVolContent() {

}

Volume* NDBVolContent::getByID(Volume* vol, QString const& id) {
    return symbols.VolumeManager__getById(vol, id, dbName);
}

int NDBVolContent::isValid(Volume* v) {
    return symbols.Volume__isValid(v);
}

QString NDBVolContent::getID(Volume* v) {
    NDB_DEBUG("calling Content::getId");
    return symbols.Content__getID(v);

}

QVariantMap NDBVolContent::getDbValues(QString const& cID) {
    char va[256];
    Volume* v = getByID((Volume*)va, cID);
    if (!v) {
        NDB_DEBUG("Volume pointer NULL");
        return QVariantMap();
    }
    QVariantMap volMap = symbols.Volume__getDbValues(v);
    Content *c = v;
    QVariantMap contentMap = symbols.Content__getDbValues(c);
    auto merged = contentMap;
    for (auto i = volMap.constBegin(); i != volMap.constEnd(); ++i) {
        merged.insert(i.key(), i.value());
    }
    return merged;
}

QStringList NDBVolContent::getBookList() {
    using std::placeholders::_1;
    currBookList.clear();
    NDB_DEBUG("calling Volume::forEach()");
    symbols.Volume__forEach(dbName, std::bind(&NDBVolContent::forEachFunc, this, _1));
    return currBookList;
}

void NDBVolContent::forEachFunc(Volume /*const&*/ *v) {
    NDB_DEBUG("entering NDBVolContent::forEachFunc");
    if (symbols.Volume__isValid(v)) {
        QString s = getID(v);
        NDB_DEBUG("got id '%s' in forEachFunc", s.toUtf8().constData());
        currBookList.append(s);
    }
}
}