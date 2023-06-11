#include "client_listener.h"

#include <QDebug>

namespace Client {
Listener::Listener(quint16 portListener, QObject* parent)
    : QObject { parent }
{
    qDebug() << "Starting listener on port" << portListener;

    _server = new QTcpServer(this);
    _server->listen(QHostAddress::Any, portListener);

    if (!_server->isListening()) {
        qCritical() << "Could not bind to" << _server->serverAddress() << _server->serverPort();
    }

    connect(_server, &QTcpServer::newConnection, this, &Listener::handleClientConnection);
}

Listener::~Listener()
{
    qDebug() << "Destroying listener";

    _server->close();
    _server->deleteLater();
}

void Listener::handleClientConnection()
{
    qDebug() << "New connections available";

    while (_server->hasPendingConnections()) {
        std::shared_ptr<Osc::Client> client(new Osc::Client(_server->nextPendingConnection()));

        emit clientConnected(client);
    }
}
}
