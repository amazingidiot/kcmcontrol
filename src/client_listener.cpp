#include "client_listener.h"
#include "client_manager.h"
#include <QDebug>

namespace Client {
Listener::Listener(Manager* parent)
    : QObject { parent }
{
    _manager = parent;
}

Listener::~Listener()
{
    qDebug() << "Destroying listener";

    _server->close();
    _server->deleteLater();
}

int Listener::init(quint16 portListener)
{
    qDebug() << "Starting listener on port" << portListener;

    _server = new QTcpServer(this);
    _server->listen(QHostAddress::Any, portListener);

    if (!_server->isListening()) {
        qCritical() << "Could not bind to" << _server->serverAddress() << _server->serverPort();

        return -1;
    }

    connect(_server, &QTcpServer::newConnection, this, &Listener::onClientConnection);

    return 0;
}

void Listener::onClientConnection()
{
    qDebug() << "New connections available";

    while (_server->hasPendingConnections()) {
        Osc::Client* client = new Osc::Client(_manager, _server->nextPendingConnection());

        emit clientConnected(client);
    }
}
}
