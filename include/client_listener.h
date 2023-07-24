#pragma once

#include "osc_client.h"
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

namespace Client {
class Manager;

class Listener : public QObject {
    Q_OBJECT
public:
    explicit Listener(Manager* parent);
    ~Listener();

    int init(quint16 portListener);
signals:
    void clientConnected(Osc::Client* client);

private:
    Manager* _manager;
    QTcpServer* _server;

private slots:
    void onClientConnection();
};
} // namespace Client
