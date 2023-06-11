#pragma once

#include <QHash>
#include <QHostAddress>
#include <QList>
#include <QObject>
#include <QRegularExpression>
#include <QTcpSocket>
#include <QtGlobal>

#include <functional>
#include <memory>

#include "osc_client.h"
#include "osc_message.h"

namespace Osc {

class Server : public QObject {
    Q_OBJECT

private:
    QTcpSocket* _socket;
    bool _enabled = false;
    quint16 _port = 1;

    QList<std::shared_ptr<Osc::Client>> _clients;

    QList<std::shared_ptr<Osc::Endpoint>> _endpoints;

public:
    Server();
    ~Server();

    quint16 port();

    bool enabled();

public slots:
    void setPort(quint16 value);
    void setEnabled(bool value);

    void receiveDatagram();

    void sendOscMessage(std::shared_ptr<Osc::Message> message);
    void sendOscMessage(std::shared_ptr<Osc::Message> message,
        std::shared_ptr<Osc::Client> client);

    void endpoint_heartbeat(std::shared_ptr<Osc::Message> received_message);

signals:
    void portChanged(quint16 value);
    bool enabledChanged(bool value);
};

} // namespace Osc
