#ifndef NDB_METADA_H
#define NDB_METADA_H

#include <functional>
#include <QObject>
#include <QSet>
#include <QStringList>
#include <QVariantMap>

#include "ndb.h"

typedef void Volume; // Inherits Content
typedef QObject VolumeManager; // The VolumeManager is a QObject though
typedef void Device;

/* Volume/Content objects appear to be only 8 bytes. They
   hold a pointer to a much larger ContentPrivate/VolumePrivate
   object (VolumePrivate is at least 400 bytes).
   Setting 16 bytes just to be safe here. */
#define VOLUME_SIZE 16

namespace NDB {

typedef void(*dstor_ptr_t)(void*);

struct VolVtable {
    dstor_ptr_t volume_dstor;
};

class NDBVolume {
    public:
        enum Result initResult;
        Volume* getVolume() { return (Volume*)&vol; }
        
        NDBVolume();
        ~NDBVolume();
    private:
        // Volume contains a vptr and a VolumePrivate ptr
        struct {
            VolVtable* vptr = nullptr;
            void* vol_private = nullptr;
        } vol;
        void* Volume_vtable;
        void* Content_vtable;
};

class NDBMetadata : public QObject {
    Q_OBJECT

    public:
        enum Result initResult;
        NDBMetadata(QObject* parent);
        ~NDBMetadata();

        QVariantMap getMetadata(QString const& cID);
        Result setMetadata(QString const& cID, QVariantMap md);
        QStringList getBookListAll();
        QStringList getBookListDownloaded();
        QStringList getBookListSideloaded();

        QString ATTRIBUTION;
        QString CONTENT_ID;
        QString CONTENT_TYPE;
        QString DESCRIPTION;
        QString FILE_SIZE;
        QString IS_DOWNLOADED;
        QString SERIES;
        QString SERIES_ID;
        QString SERIES_NUMBER;
        QString SERIES_NUMBER_FLOAT;
        QString SUBTITLE;

    private:
        QString* dbName = nullptr;
        Device* device = nullptr;
        struct {
            // Getting Volume's
            VolumeManager* (*VolumeManager__sharedInstance)();
            Volume*        (*VolumeManager__getById)(Volume* vol, QString const& id, QString const& dbName);
            void           (*VolumeManager__forEach)(QString const& dbName, std::function<void(Volume /*const&*/ *v)> f);
            int            (*Volume__isValid)(Volume* _this);
            QVariantMap    (*Volume__getDbValues)(Volume* volume);
            int            (*Volume__save)(Volume* _this, Device* device);
            void           (*Volume__setAttribute)(Volume* _this, QString const& key, QVariant const& val);
        } symbols;

        QSet<QString> availableAttr;
        
        Volume* getByID(Volume* vol, QString const& id);
        bool volIsValid(Volume* v);
        QVariantMap getMetadata(Volume* v);
        QStringList getBookList(std::function<bool (Volume*)> filter);
};
}

#endif
