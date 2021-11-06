#ifndef NDB_VOLUME_H
#define NDB_VOLUME_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QVariant>
#include <QPointer>
#include "ndb.h"

typedef void Content; // It doesn't appear to be a QObject
typedef Content Volume; // Inherits Content
typedef QObject VolumeManager; // The VolumeManager is a QObject though
typedef void Device;

namespace NDB {
class NDBVolContent : public QObject {
    Q_OBJECT
    public:
        enum Result initResult;
        NDBVolContent(QObject* parent);
        ~NDBVolContent();

        QStringList getBookList(bool downloaded = true, bool onlySideloaded = false);
        int isValid(Volume* v);
        QVariantMap getDbValues(QString const& cID);
        QVariantMap getDbValues(Volume* v);

        void setAttribution(QString const& id, QString const& attribution);
        void setDescription(QString const& id, QString const& description);
        void setSeries(QString const& id, QString const& series);
        void setSeriesID(QString const& id, QString const& seriesID);
        void setSeriesNum(QString const& id, QString const& seriesNum);
        void setSeriesNumFloat(QString const& id, double num);
        void setSubtitle(QString const& id, QString const& subtitle);
        void commitMetadata(QString const& id);
    private:
        struct {
            // Getting Volume's
            VolumeManager *(*VolumeManager__sharedInstance)();
            Volume *(*VolumeManager__getById)(Volume* vol, QString const& id, QString const& dbName);
            void (*Volume__forEach)(QString const& dbName, std::function<void(Volume /*const&*/ *v)> f);
            int (*Volume__isValid)(Volume* _this);
            QVariantMap (*Content__getDbValues)(Content* content);
            QVariantMap (*Volume__getDbValues)(Volume* volume);
            int (*Volume__save)(Volume* _this, Device* device);
            void (*Volume__setAttribute)(Volume* _this, QString const& key, QVariant const& val);
        } symbols;
        struct {
            QString* attribution;
            QString* contentID;
            QString* contentType;
            QString* description;
            QString* filesize;
            QString* isDownloaded;
            QString* series;
            QString* seriesID;
            QString* seriesNum;
            QString* seriesNumFloat;
            QString* subtitle;
        } attr;
        QStringList currBookList;
        struct {
            bool downloaded;
            bool onlySideloaded;
        } currBlSettings;
        Device* device;
        void forEachFunc(Volume /*const&*/ *v);
        Volume* getByID(Volume* vol, QString const& id);
        QString dbName;

        QMap<QString, QVariantMap> pending;
        void addToPending(QString const& id, QString const& key, QVariant const& val);
};
} // namespace NDB
#endif // NDB_VOLUME_H
