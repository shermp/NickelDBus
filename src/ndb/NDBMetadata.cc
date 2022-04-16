#include <functional>
#include <QMetaType>

#include <NickelHook.h>
#include "util.h"
#include "NDBMetadata.h"

#define VOLUME_SIZE 256 // Unsure of the exact value for this, 256 bytes seems large enough though

namespace NDB {

NDBMetadata::~NDBMetadata() {}

NDBMetadata::NDBMetadata(QObject* parent) : QObject(parent) {
    initResult = Ok;
    resolveSymbolRTLD("_ZN13VolumeManager14sharedInstanceEv", nh_symoutptr(symbols.VolumeManager__sharedInstance));
    if (!symbols.VolumeManager__sharedInstance) {
        initResult = SymbolError;
        return;
    }
    Device *(*Device__getCurrentDevice)();
    resolveSymbolRTLD("_ZN6Device16getCurrentDeviceEv", nh_symoutptr(Device__getCurrentDevice));
    if (!Device__getCurrentDevice) {
        return;
    }

    device = Device__getCurrentDevice();
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
    resolveSymbolRTLD("_ZNK7Content11getDbValuesEv", nh_symoutptr(symbols.Content__getDbValues));
    resolveSymbolRTLD("_ZNK6Volume11getDbValuesEv", nh_symoutptr(symbols.Volume__getDbValues));
    resolveSymbolRTLD("_ZN13VolumeManager7forEachERK7QStringRKSt8functionIFvRK6VolumeEE", nh_symoutptr(symbols.Volume__forEach));
    resolveSymbolRTLD("_ZN6Volume12setAttributeERK7QStringRK8QVariant", nh_symoutptr(symbols.Volume__setAttribute));
    resolveSymbolRTLD("_ZN6Volume4saveERK6Device", nh_symoutptr(symbols.Volume__save));
    if (!symbols.VolumeManager__getById || 
        !symbols.Content__getDbValues ||
        !symbols.Volume__getDbValues ||
        !symbols.Volume__isValid ||
        !symbols.Volume__forEach ||
        !symbols.Volume__setAttribute ||
        !symbols.Volume__save) {
        initResult = SymbolError;
        return;
    }
    
    auto i = nickelAttr.begin();
    while (i != nickelAttr.end()) {
        auto attr_name = i.key().toUtf8().constData();
        resolveSymbolRTLD(attr_name, nh_symoutptr(i.value()));
        if (i.value() == nullptr) {
            nh_log("Attribute not found: %s", attr_name);
            i = nickelAttr.erase(i);
        } else {
            ++i;
        }
    }
}

bool NDBMetadata::volIsValid(Volume* v) {
    return symbols.Volume__isValid(v) != 0;
}

Volume* NDBMetadata::getByID(Volume* vol, QString const& id) {
    return symbols.VolumeManager__getById(vol, id, dbName);
}

QVariantMap NDBMetadata::getMetadata(QString const& cID) {
    char va[VOLUME_SIZE];
    Volume* v = getByID((Volume*)va, cID);
    return getMetadata(v);
}

QVariantMap NDBMetadata::getMetadata(Volume* v) {
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

QStringList NDBMetadata::getBookList(bool downloaded, bool onlySideloaded) {
    QStringList bookList = {};
    NDB_DEBUG("calling Volume::forEach()");
    std::function<void(Volume *v)> fe = [&](Volume *v){
        NDB_DEBUG("Entering Lambda");
        bool addToList = true;
        if (volIsValid(v)) {
            QVariantMap values = getMetadata(v);
            QString cID = values[attr.contentID].toString();
            NDB_DEBUG("got id '%s' in forEachFunc", cID.toUtf8().constData());
            bool isDownloaded = values[attr.isDownloaded].toBool();
            int filesize = values[attr.fileSize].toInt();
            if (downloaded && (!isDownloaded || filesize <= 0)) {
                addToList = false;
            }
            if (onlySideloaded && !cID.startsWith("file:///")) {
                addToList = false;
            }
            if (addToList) {
                bookList.append(cID);
            }
        }
    };
    symbols.Volume__forEach(dbName, fe);
    return bookList;
}

// void NDBMetadata::forEachFunc(Volume /*const&*/ *v) {
//     NDB_DEBUG("entering NDBVolContent::forEachFunc");
//     bool addToList = true;
//     if (volIsValid(v)) {
//         QVariantMap values = getMetadata(v);
//         QString cID = values[attr.contentID].toString();
//         NDB_DEBUG("got id '%s' in forEachFunc", cID.toUtf8().constData());
//         bool isDownloaded = values[attr.isDownloaded].toBool();
//         int filesize = values[attr.fileSize].toInt();
//         if (currBL.downloaded && (!isDownloaded || filesize <= 0)) {
//             addToList = false;
//         }
//         if (currBL.onlySideloaded && !cID.startsWith("file:///")) {
//             addToList = false;
//         }
//         if (addToList) {
//             currBL.list.append(cID);
//         }
//     }
// }

Result NDBMetadata::setMetadata(QString const& cID, QVariantMap md) {
    char va[VOLUME_SIZE];
    Volume* v = getByID((Volume*)va, cID);
    NDB_ASSERT(NullError, v, "Error getting Volume for %s", cID.toUtf8().constData());
    NDB_ASSERT(VolumeError, volIsValid(v), "Volume is not valid for %s", cID.toUtf8().constData());
    for (auto i = md.constBegin(); i != md.constEnd(); ++i) {
        QString key = "ATTRIBUTE_" + i.key().toUpper();
        const QVariant val = i.value();
        // Skip keys that don't exist, and ContentID
        if (!nickelAttr.contains(key) || key == attr.contentID) {
            NDB_DEBUG("Skipping %s", key.toUtf8().constData());
            continue;
        }
        // Check metadata types match expected
        auto type = val.userType();
        bool validType = true;
        if (key == attr.seriesNumFloat) {
            if (type != QMetaType::Double && type != QMetaType::Float) {
                validType = false;
            }
        } else if (type != QMetaType::QString) {
            validType = false;
        }
        NDB_ASSERT(TypeError, validType, "Unexpected type for key %s", key.toUtf8().constData());
        symbols.Volume__setAttribute(v, key, val);
    }
    int res = symbols.Volume__save(v, device);
    NDB_DEBUG("Volume__save returned with val %d", res);
    return Ok;
}

}
