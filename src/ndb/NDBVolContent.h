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

        QStringList getBookList();
        int isValid(Volume* v);
        QString getID(Volume* v);
        QVariantMap getDbValues(QString const& cID);
    private:
        struct {
            // Getting Volume's
            VolumeManager *(*VolumeManager__sharedInstance)();
            Volume *(*VolumeManager__getById)(Volume* vol, QString const& id, QString const& dbName);
            void (*Volume__forEach)(QString const& dbName, std::function<void(Volume /*const&*/ *v)> f);
            int (*Volume__isValid)(Volume* _this);
            // get metadata from Content and Volume objects
            QString (*Content__getID)(Content* c);
            QVariantMap (*Content__getDbValues)(Content* content);
            QVariantMap (*Volume__getDbValues)(Volume* volume);
        } symbols;
        QStringList currBookList;
        void forEachFunc(Volume /*const&*/ *v);
        Volume* getByID(Volume* vol, QString const& id);
        QString dbName;
};
} // namespace NDB
#endif // NDB_VOLUME_H
