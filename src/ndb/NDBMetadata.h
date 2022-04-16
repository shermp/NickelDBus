#ifndef NDB_METADA_H
#define NDB_METADA_H

#include <QObject>
#include <QHash>
#include <QStringList>
#include <QVariantMap>

#include "ndb.h"

typedef void Content; // It doesn't appear to be a QObject
typedef Content Volume; // Inherits Content
typedef QObject VolumeManager; // The VolumeManager is a QObject though
typedef void Device;

namespace NDB {

class NDBMetadata : public QObject {
    Q_OBJECT

    public:
        enum Result initResult;
        NDBMetadata(QObject* parent);
        ~NDBMetadata();

        QVariantMap getMetadata(QString const& cID);
        Result setMetadata(QString const& cID, QVariantMap md);
        QStringList getBookList(bool downloaded, bool onlySideloaded);

    private:
        QString dbName;
        Device* device;
        struct {
            // Getting Volume's
            VolumeManager* (*VolumeManager__sharedInstance)();
            Volume*        (*VolumeManager__getById)(Volume* vol, QString const& id, QString const& dbName);
            void           (*Volume__forEach)(QString const& dbName, std::function<void(Volume /*const&*/ *v)> const& f);
            int            (*Volume__isValid)(Volume* _this);
            QVariantMap    (*Content__getDbValues)(Content* content);
            QVariantMap    (*Volume__getDbValues)(Volume* volume);
            int            (*Volume__save)(Volume* _this, Device* device);
            void           (*Volume__setAttribute)(Volume* _this, QString const& key, QVariant const& val);
        } symbols;

        struct {
            QString attribution = "ATTRIBUTE_ATTRIBUTION";
            QString contentID = "ATTRIBUTE_CONTENT_ID";
            QString contentType = "ATTRIBUTE_CONTENT_ID";
            QString description = "ATTRIBUTE_DESCRIPTION";
            QString fileSize = "ATTRIBUTE_FILE_SIZE";
            QString isDownloaded = "ATTRIBUTE_IS_DOWNLOADED";
            QString series = "ATTRIBUTE_SERIES";
            QString seriesID = "ATTRIBUTE_SERIES_ID";
            QString seriesNum = "ATTRIBUTE_SERIES_NUMBER";
            QString seriesNumFloat = "ATTRIBUTE_SERIES_NUMBER_FLOAT";
            QString subtitle = "ATTRIBUTE_SUBTITLE";
        } attr;

        QHash<QString, QString*> nickelAttr = {
            {attr.attribution, nullptr},
            {attr.contentID, nullptr},
            {attr.contentType, nullptr},
            {attr.description, nullptr},
            {attr.fileSize, nullptr},
            {attr.isDownloaded, nullptr},
            {attr.series, nullptr},
            {attr.seriesID, nullptr},
            {attr.seriesNum, nullptr},
            {attr.seriesNumFloat, nullptr},
            {attr.subtitle, nullptr}
        };

        struct {
            bool downloaded;
            bool onlySideloaded;
            QStringList list = {};
        } currBL;
        Volume* getByID(Volume* vol, QString const& id);
        bool volIsValid(Volume* v);
        QVariantMap getMetadata(Volume* v);
        //void forEachFunc(Volume /*const&*/ *v);
};
}

#endif
