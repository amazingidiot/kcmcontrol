#include "client_manager.h"
#include "sigwatch.h"
#include <QCoreApplication>
#include <QLocale>
#include <QSettings>
#include <QtDebug>

int main(int argc, char* argv[])
{
    QCoreApplication a(argc, argv);

    qSetMessagePattern("[%{time} %{type}] %{message}");

    UnixSignalWatcher watcher;

    watcher.watchForSignal(SIGINT);
    watcher.watchForSignal(SIGTERM);

    QObject::connect(&watcher, &UnixSignalWatcher::unixSignal, &a,
        &QCoreApplication::quit);

    Client::Manager manager;

    if (manager.init() < 0) {
        qCritical() << "Could not initialize manager";

        return -1;
    }

    int result = a.exec();

    qInfo() << "Quitting KCMControl";

    return result;
}
