#include <QCoreApplication>
#include <QLocale>
#include <QSettings>
#include <QtDebug>

#include "client_manager.h"
#include "sigwatch.h"

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

    int result = a.exec();

    qInfo() << "Quitting KCMControl";

    return result;
}
