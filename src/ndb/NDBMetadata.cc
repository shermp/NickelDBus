#include <functional>
#include <QMetaType>

#include <NickelHook.h>
#include "util.h"
#include "NDBMetadata.h"

#define NDB_RESOLVE_ATTR(attr, required) if (!resolve_attr(attr, "ATTRIBUTE_" #attr, required)) return

namespace NDB {

/* Volume contains a large heap allocated VolumePrivate member variable that needs to be cleaned up
   Thankfully it has a virtual destructor. */
NDBVolume::~NDBVolume() {
    // Don't try and call the destructor if we never got a valid object
    NDB_ASSERT_RET(vptr != nullptr && vol_private != nullptr, "vptr or VolumePrivate not set on Volume.");
    void *Volume_vtable = nullptr;
    resolveSymbolRTLD("_ZTV6Volume", nh_symoutptr(Volume_vtable));
    NDB_ASSERT_RET(Volume_vtable, "Volume vtable not found");

    // Shamelessly stolen from NickelMenu
    #define vtable_target(x) reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(x)+8)
    NDB_DEBUG(" Value of Volume vtable: %p", vtable_target(Volume_vtable));
    NDB_DEBUG("  Value of current vptr: %p", vptr);
    NDB_ASSERT_RET(vptr == vtable_target(Volume_vtable), "unexpected vtable layout (expected class to start with a pointer to 8 bytes into the vtable)");

    vptr->volume_dstor(this);
    NDB_DEBUG("Virtual Volume Destructor invoked.");
    return;
}

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
    resolveSymbolRTLD("_ZN13VolumeManager7forEachERK7QStringRKSt8functionIFvRK6VolumeEE", nh_symoutptr(symbols.VolumeManager__forEach));
    resolveSymbolRTLD("_ZNK6Volume7isValidEv", nh_symoutptr(symbols.Volume__isValid));
    resolveSymbolRTLD("_ZNK6Volume11getDbValuesEv", nh_symoutptr(symbols.Volume__getDbValues));
    resolveSymbolRTLD("_ZN6Volume12setAttributeERK7QStringRK8QVariant", nh_symoutptr(symbols.Volume__setAttribute));
    resolveSymbolRTLD("_ZN6Volume4saveERK6Device", nh_symoutptr(symbols.Volume__save));
    if (!symbols.VolumeManager__getById || 
        !symbols.VolumeManager__forEach ||
        !symbols.Volume__getDbValues ||
        !symbols.Volume__isValid ||
        !symbols.Volume__setAttribute ||
        !symbols.Volume__save) {
        initResult = SymbolError;
        return;
    }

    auto resolve_attr = [&](QString& attr, const char* name, bool required) {
        QString *tmp_attr = nullptr;
        resolveSymbolRTLD(name, nh_symoutptr(tmp_attr));
        if (required && !tmp_attr) {
            nh_log("could not dlsym attribute %s", name);
            initResult = SymbolError;
            return false;
        }
        if (tmp_attr) {
            attr = *tmp_attr;
            availableAttr.insert(attr);
        }
        return true;
    };
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

NDBVolume NDBMetadata::getByID(QString const& id) {
    return symbols.VolumeManager__getById(id, *dbName);
}

QVariantMap NDBMetadata::getMetadata(QString const& cID) {
    // NDBVolume vol;
    // NDB_ASSERT(QVariantMap(), vol.initResult == Ok, "Unable to construct Volume");
    NDBVolume v = getByID(cID);
    NDB_DEBUG("Volume vptr = %p, VolumePrivate = %p", v.vptr, v.vol_private);
    NDB_DEBUG("Volume is %svalid", volIsValid((Volume*)&v) ? "" : "not ");
    return getMetadata(reinterpret_cast<Volume*>(&v));
}

QVariantMap NDBMetadata::getMetadata(Volume* v) {
    if (!v) {
        NDB_DEBUG("Volume pointer NULL");
        return QVariantMap();
    }
    return symbols.Volume__getDbValues(v);
}

QStringList NDBMetadata::getBookList(std::function<bool (Volume*)> filter) {
    NDB_DEBUG("calling Volume::forEach()");
    QStringList bookList = {};
    symbols.VolumeManager__forEach(*dbName, [&](Volume *v) {
        if (volIsValid(v) && filter(v)) {
            QVariantMap values = getMetadata(v);
            QString cID = values[CONTENT_ID].toString();
            bookList.append(cID);
        }
    });
    return bookList;
}

QStringList NDBMetadata::getBookListAll() {
    return getBookList([]([[gnu::unused]] Volume* v){ return true; });
}

QStringList NDBMetadata::getBookListDownloaded() {
    return getBookList([&](Volume* v) {
        QVariantMap values = getMetadata(v);
        bool isDownloaded = values[IS_DOWNLOADED].toBool();
        int filesize = values[FILE_SIZE].toInt();
        return isDownloaded && filesize > 0;
    });
}

QStringList NDBMetadata::getBookListSideloaded() {
    return getBookList([&](Volume* v) {
        QVariantMap values = getMetadata(v);
        QString cID = values[CONTENT_ID].toString();
        return cID.startsWith("file:///");
    });
}

Result NDBMetadata::setMetadata(QString const& cID, QVariantMap md) {
    NDBVolume vol = getByID(cID);
    Volume* v = reinterpret_cast<Volume*>(&vol);
    NDB_ASSERT(VolumeError, volIsValid(v), "Volume is not valid for %s", cID.toUtf8().constData());
    for (auto i = md.constBegin(); i != md.constEnd(); ++i) {
        const QString key = i.key();
        const QVariant val = i.value();
        // Skip keys that don't exist, and ContentID
        if (!availableAttr.contains(key) || key == CONTENT_ID) {
            NDB_DEBUG("Skipping %s", key.toUtf8().constData());
            continue;
        }
        // Check metadata types match expected
        auto type = val.userType();
        bool validType = true;
        if (key == SERIES_NUMBER_FLOAT) {
            if (type != QMetaType::Double && type != QMetaType::Float) {
                validType = false;
            }
        } else if (key == FILE_SIZE && type != QMetaType::Int) {
            validType = false;
        } else if (type != QMetaType::QString) {
            validType = false;
        }
        NDB_ASSERT(TypeError, validType, "Unexpected type for key %s", key.toUtf8().constData());
        symbols.Volume__setAttribute(v, key, val);
    }
    int res = symbols.Volume__save(v, device);
    NDB_ASSERT(MetadataError, res, "error saving metadata for id %s", cID.toUtf8().constData());
    NDB_DEBUG("Volume__save returned with val %d", res);
    return Ok;
}

}
