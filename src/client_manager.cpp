#include "client_manager.h"

namespace Client {
Manager::Manager(quint16 portListener, QString serviceName, QObject* parent)
    : QObject { parent }
{
    _listener = new Client::Listener(portListener, this);
    _watcher = new Client::Watcher(serviceName, this);
    _processor = new Osc::Processor(this);

    connect(_listener, &Listener::clientConnected, this, &Manager::addClient);
}

Manager::~Manager()
{
    qDebug() << "Destroying manager";

    for (int i = 0; i < _clients.length(); i++) {
        Osc::Client* client = _clients.at(i);

        delete client;
    }
}

void Manager::addClient(Osc::Client* client)
{
    _clients.append(client);

    connect(client, SIGNAL(disconnected(Osc::Client*)), this, SLOT(removeClient(Osc::Client*)));
}

void Manager::removeClient(Osc::Client* client)
{
    for (int i = 0; i < _clients.length(); i++) {
        if (_clients.at(i) == client) {
            _clients.removeAt(i);
            break;
        }
    }
}
} // namespace Client
