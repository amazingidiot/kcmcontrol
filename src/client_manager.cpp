#include "client_manager.h"

namespace Client {
Manager::Manager(QObject* parent)
    : QObject { parent }
{
    _processor = new Osc::Processor(this);
    _listener = new Client::Listener(this);
    _watcher = new Client::Watcher(this);
}

Manager::~Manager()
{
    qDebug() << "Destroying manager";

    for (int i = 0; i < _clients.length(); i++) {
        Osc::Client* client = _clients.at(i);

        delete client;
    }
}

int Manager::init(quint16 portListener, QString serviceName)
{
    if (_processor->init(_watcher) < 0) {
        qCritical() << "Could not initialize processor";
        return -1;
    }

    if (_listener->init(portListener) < 0) {
        qCritical() << "Could not initialize listener";
        return -1;
    }

    if (_watcher->init(serviceName) < 0) {
        qCritical() << "Could not initialize watcher";
        return -1;
    }

    connect(_listener, &Listener::clientConnected, this, &Manager::addClient);

    return 0;
}

Osc::Client* Manager::getClient(QHostAddress address, quint16 port)
{
    for (auto client : _clients) {
        if (client->peerPort() == port) {
            if (client->peerAddress() == address) {
                return client;
            }
        }
    }

    return nullptr;
}

void Manager::addClient(Osc::Client* client)
{
    _clients.append(client);

    connect(client, SIGNAL(disconnected(Osc::Client*)), this, SLOT(removeClient(Osc::Client*)));
    connect(client, &Osc::Client::received, _processor, &Osc::Processor::process);
}

void Manager::removeClient(Osc::Client* client)
{
    for (int i = 0; i < _clients.length(); i++) {
        if (_clients.at(i) == client) {
            qDebug() << "Found client to remove at index" << i;
            _clients.removeAt(i);
            client->deleteLater();
            break;
        }
    }
}
} // namespace Client
