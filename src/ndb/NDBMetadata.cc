#include <functional>
#include <QMetaType>

#include <NickelHook.h>
#include "util.h"
#include "NDBMetadata.h"

#define VOLUME_SIZE 256 // Unsure of the exact value for this, 256 bytes seems large enough though

#define NDB_RESOLVE_ATTR(attr, required) resolveSymbolRTLD("ATTRIBUTE_" #attr, nh_symoutptr(attr)); \
    if ((required) && !(attr)) {nh_log("could not dlsym attribute %s", #attr); initResult = SymbolError; return;} \
    if ((attr)) availableAttr.insert(*attr);

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
        initResult = SymbolError;
        return;
    }

    device = Device__getCurrentDevice();
    if (!device) {
        nh_log("could not get current device");
        initResult = NullError;
        return;
    }
    QString *(*Device__getDbName)(Device* _this);
    resolveSymbolRTLD("_ZNK6Device9getDbNameEv", nh_symoutptr(Device__getDbName));
    if (!Device__getDbName) {
        initResult = SymbolError;
        return;
    }
    dbName = Device__getDbName(device);
    if (!dbName) {
        nh_log("could not get DB name");
        initResult = NullError;
        return;
    }
    nh_log("DB name is %s", dbName->toUtf8().constData());

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
    // Getting/setting Volume/Content attribute/keys
    NDB_RESOLVE_ATTR(ATTRIBUTION, true);
    NDB_RESOLVE_ATTR(CONTENT_ID, true);
    NDB_RESOLVE_ATTR(CONTENT_TYPE, true);
    NDB_RESOLVE_ATTR(DESCRIPTION, true);
    NDB_RESOLVE_ATTR(FILE_SIZE, true);
    NDB_RESOLVE_ATTR(IS_DOWNLOADED, true);
    NDB_RESOLVE_ATTR(SERIES, true);
    NDB_RESOLVE_ATTR(SERIES_ID, false);
    NDB_RESOLVE_ATTR(SERIES_NUMBER, true);
    NDB_RESOLVE_ATTR(SERIES_NUMBER_FLOAT, true);
    NDB_RESOLVE_ATTR(SUBTITLE, true);
}

bool NDBMetadata::volIsValid(Volume* v) {
    return symbols.Volume__isValid(v) != 0;
}

Volume* NDBMetadata::getByID(Volume* vol, QString const& id) {
    return symbols.VolumeManager__getById(vol, id, *dbName);
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
    NDB_DEBUG("calling Volume::forEach()");
    QStringList bookList = {};
    symbols.Volume__forEach(*dbName, [&](Volume *v) {
        bool addToList = true;
        if (volIsValid(v)) {
            QVariantMap values = getMetadata(v);
            QString cID = values[*CONTENT_ID].toString();
            bool isDownloaded = values[*IS_DOWNLOADED].toBool();
            int filesize = values[*FILE_SIZE].toInt();
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
    });
    return bookList;
}

Result NDBMetadata::setMetadata(QString const& cID, QVariantMap md) {
    char va[VOLUME_SIZE];
    Volume* v = getByID((Volume*)va, cID);
    NDB_ASSERT(NullError, v, "Error getting Volume for %s", cID.toUtf8().constData());
    NDB_ASSERT(VolumeError, volIsValid(v), "Volume is not valid for %s", cID.toUtf8().constData());
    for (auto i = md.constBegin(); i != md.constEnd(); ++i) {
        const QString key = i.key();
        const QVariant val = i.value();
        // Skip keys that don't exist, and ContentID
        if (!availableAttr.contains(key) || key == *CONTENT_ID) {
            NDB_DEBUG("Skipping %s", key.toUtf8().constData());
            continue;
        }
        // Check metadata types match expected
        auto type = val.userType();
        bool validType = true;
        if (key == *SERIES_NUMBER_FLOAT) {
            if (type != QMetaType::Double && type != QMetaType::Float) {
                validType = false;
            }
        } else if (key == *FILE_SIZE && type != QMetaType::Int) {
            validType = false;
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
