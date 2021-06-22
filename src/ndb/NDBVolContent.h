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

        Volume* getByID(QString const& id);
        QStringList getBookList();
        int isValid(Volume* v);
        QString getID(Volume* v);
        QMap<QString, QVariant> getDbValues(QString const& cID);
    private:
        struct {
            // Getting Volume's
            Volume *(*VolumeManager__getById)(VolumeManager* _this, QString const& id, QString const& dbName);
            void (*Volume__forEach)(QString const& dbName, std::function<void(Volume /*const&*/ *v)> f);
            int (*Volume__isValid)(Volume* _this);
            // get metadata from Content and Volume objects
            QString (*Content__getID)(Content* c);
            QMap<QString, QVariant> (*Content__getDbValues)(Content* content);
            QMap<QString, QVariant> (*Volume__getDbValues)(Volume* volume);
        } symbols;
        QStringList currBookList;
        void forEachFunc(Volume /*const&*/ *v);
        QPointer<VolumeManager> vm;
        QString* dbName;

        QMap<QString, QVariant> getContentDbValues(Content* content);
        QMap<QString, QVariant> getVolumeDbValues(Volume* volume);
};
} // namespace NDB
#endif // NDB_VOLUME_H
