#include <QCoreApplication>
#include <QLocale>
#include <QSettings>
#include <QTranslator>
#include <QtDebug>

#include "osc_server.h"
#include "sigwatch.h"

int main(int argc, char* argv[]) {
  QCoreApplication a(argc, argv);

  qSetMessagePattern("[%{time} %{type}] %{message}");

  QTranslator translator;
  const QStringList uiLanguages = QLocale::system().uiLanguages();
  for (const QString& locale : uiLanguages) {
    qDebug() << QCoreApplication::tr("Trying to load language %1")
                    .arg(QLocale(locale).name());

    QString baseName = "elzac_" + QLocale(locale).name();

    if (translator.load(":/i18n/" + baseName)) {
      qDebug() << QCoreApplication::tr("Loaded language %1")
                      .arg(translator.language());

      a.installTranslator(&translator);
      break;
    }
  }

  UnixSignalWatcher watcher;

  watcher.watchForSignal(SIGINT);
  watcher.watchForSignal(SIGTERM);

  QObject::connect(&watcher, &UnixSignalWatcher::unixSignal, &a,
                   &QCoreApplication::quit);

  Osc::Server server;

  server.setPort(31032);
  server.setEnabled(true);

  int result = a.exec();

  qInfo() << QCoreApplication::tr("Quitting elzac");

  return result;
}
