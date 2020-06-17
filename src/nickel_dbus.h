#ifndef NICKEL_DBUS_H
#define NICKEL_DBUS_H
#include <QObject>
#include <QString>
#include <QtDBus>

class NickelDBus : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "local.shermp.nickeldbus")
    public:
        void *libnickel;
        bool dbusRegSucceeded;
        NickelDBus(QObject* parent);
        ~NickelDBus();
        // bool registerDBus();
        bool connectSignals(char **err_out);
    Q_SIGNALS:
        void pfmDoneProccessing(bool done = true);
    
    public Q_SLOTS:
        QString version();
        bool pfmRescanBooksFull();
};

#endif