#pragma once

#include "client_listener.h"
#include "client_watcher.h"
#include "osc_client.h"
#include "osc_processor.h"

#include <QHash>
#include <QObject>
#include <QTcpSocket>

namespace Client {
class Manager : public QObject {
    Q_OBJECT
public:
    explicit Manager(quint16 portListener = 8000, QString serviceName = "kcmcontrol", QObject* parent = nullptr);
    ~Manager();

    Osc::Client* getClient(QHostAddress address, quint16 port);

signals:
    void clientAdded(Osc::Client* client);

public slots:
    void addClient(Osc::Client* client);
    void removeClient(Osc::Client *client);

private:
    QList<Osc::Client*> _clients;

    Watcher *_watcher;
    Listener* _listener;
    Osc::Processor* _processor;
};
}
