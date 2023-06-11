#pragma once

#include "osc_client.h"

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

#include <memory>

namespace Client {
class Listener : public QObject {
    Q_OBJECT
public:
    explicit Listener(quint16 portListener, QObject* parent = nullptr);
    ~Listener();
signals:
    void clientConnected(std::shared_ptr<Osc::Client> client);

private:
    QTcpServer *_server;

private slots:
    void handleClientConnection();
};
}
