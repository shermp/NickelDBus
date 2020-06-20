#ifndef NICKEL_DBUS_H
#define NICKEL_DBUS_H
#include <QObject>
#include <QString>
#include <QSet>
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
        void connectSignals();
        bool testAssert(bool test);
    Q_SIGNALS:
        void pfmDoneProccessing(bool done = true);
    
    public Q_SLOTS:
        QString version();
        bool signalConnected(const QString &signal_name);
        bool pfmRescanBooksFull();
    private:
        QSet<QString> connectedSignals;
};

#endif