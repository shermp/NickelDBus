#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>

#include "ndb_cli.h"

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("qndb");
    QCoreApplication::setApplicationVersion("0.1");

    QCommandLineParser parser;
    parser.setApplicationDescription("Qt CLI for NickelDBus");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument("arguments", "Arguments to pass to method. Have no affect when a method is not set.", "[args...]");

    QCommandLineOption signalOption(QStringList() << "s" << "signal", "Wait for signal, and prints its output, if any.", "signal name");
    QCommandLineOption timeoutOption(QStringList() << "t" << "timeout", "Signal timeout in milliseconds.", "timeout ms");
    QCommandLineOption methodOption(QStringList() << "m" << "method", "Method to invoke.", "method name");
    QCommandLineOption apiOption(QStringList() << "a" << "api", "Print API usage");
    parser.addOption(signalOption);
    parser.addOption(timeoutOption);
    parser.addOption(methodOption);
    parser.addOption(apiOption);

    parser.process(app);

    const QStringList methodArgs = parser.positionalArguments();

    com::github::shermp::nickeldbus ndb("com.github.shermp.nickeldbus", "/nickeldbus", QDBusConnection::systemBus(), &app);
    NDBCli cli(&app, &ndb);

    cli.setPrintAPI(parser.isSet(apiOption));
    if (parser.isSet(signalOption)) {
        cli.setSignalNames(parser.values(signalOption));
    }
    if (parser.isSet(methodOption)) {
        cli.setMethodName(parser.value(methodOption));
    }
    if (parser.isSet(timeoutOption)) {
        int timeout = -1;
        bool timeout_ok;
        timeout = parser.value(timeoutOption).toInt(&timeout_ok);
        if (timeout_ok) {
            cli.setTimeout(timeout);
        }
    }
    cli.setMethodArgs(parser.positionalArguments());
    
    QTimer::singleShot(0, &cli, SLOT(start()));
    return app.exec();
}
