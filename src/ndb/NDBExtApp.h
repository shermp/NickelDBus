#ifndef NDB_EXT_APP_H
#define NDB_EXT_APP_H

#include <QDialog>

#include "ndb.h"

namespace NDB {
class ExtAppMode : public QObject
{
    Q_OBJECT
    public:
        ExtAppMode();
        enum Result enableExtApp();
        enum Result disableExtApp();
    private:
        QDialog dlg;
};
}

#endif // NDB_EXT_APP_H
