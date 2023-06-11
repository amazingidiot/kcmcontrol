#include "osc_server.h"
#include "osc_client.h"
#include "osc_message.h"

#include <QDebug>

#include <memory>

Osc::Server::Server()
{
    qDebug() << tr("Starting server");

    _socket = new QTcpSocket(this);

    connect(_socket, &QTcpSocket::readyRead, this, &Server::receiveDatagram);
}

Osc::Server::~Server()
{
    qInfo() << "Stopping server";

    _socket->close();

    _socket->deleteLater();
}

bool Osc::Server::enabled() { return _enabled; }

void Osc::Server::setEnabled(bool value)
{
    if (value != _enabled) {
        _enabled = value;

        if (_enabled) {
            _socket->bind(QHostAddress::LocalHost, _port);
            qInfo() << tr("Binding server to %1:%2")
                           .arg(_socket->localAddress().toString())
                           .arg(_port);
        } else {
            _socket->close();
        }
        emit enabledChanged(value);
    }
}

quint16 Osc::Server::port() { return _port; }

void Osc::Server::setPort(quint16 value)
{
    if (value != _port) {
        _port = value;

        qDebug() << tr("Port set to %1").arg(_port);

        emit portChanged(_port);
    }
}

void Osc::Server::receiveDatagram()
{
    while (_socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = _socket->receiveDatagram();

        std::shared_ptr<Osc::Message> received_message(new Osc::Message(&datagram));

        qDebug().noquote() << received_message->toString();

        foreach (std::shared_ptr<Osc::Endpoint> endpoint, _endpoints) {
            if (endpoint->pattern.match(received_message->address).hasMatch()) {
                emit endpoint->received(received_message);
            }
        }
    }
}

void Osc::Server::handleNewCard(std::shared_ptr<Alsa::Card> card)
{
    qDebug() << tr("handling new card");

    connect(card->elementList().get(), &Alsa::Ctl::ElementList::elementChanged,
        this, &Osc::Server::sendElementUpdateToAllClients);
}

void Osc::Server::sendElementUpdateToAllClients(
    std::shared_ptr<Alsa::Ctl::Element> element)
{
    qDebug() << tr("Sending update of element %1:%2")
                    .arg(element->card_index())
                    .arg(element->id());

    this->sendOscMessageToAllClients(createElementValueMessage(element));
}

void Osc::Server::sendOscMessage(std::shared_ptr<Osc::Message> message)
{
    qDebug().noquote() << message->toString();

    QNetworkDatagram datagram;

    datagram.setSender(_socket->localAddress(), _socket->localPort());
    datagram.setDestination(message->destinationAddress,
        message->destinationPort);

    datagram.setData(message->toByteArray());

    _socket->writeDatagram(datagram);
}

void Osc::Server::sendOscMessage(std::shared_ptr<Message> message,
    std::shared_ptr<Client> client)
{
    qDebug().noquote() << message->toString();

    QNetworkDatagram datagram;

    datagram.setSender(_socket->localAddress(), _socket->localPort());
    datagram.setDestination(client->address(), client->port());

    datagram.setData(message->toByteArray());

    _socket->writeDatagram(datagram);
}

void Osc::Server::sendOscMessageToAllClients(
    std::shared_ptr<Osc::Message> message)
{
    std::shared_ptr<Osc::Message> client_message(
        new Osc::Message(message->address, message->values));

    foreach (std::shared_ptr<Osc::Client> client, _clients) {
        client_message->destinationAddress = client->address();
        client_message->destinationPort = client->port();

        this->sendOscMessage(client_message);
    }
}

void Osc::Server::updateClientList()
{
    QMutableListIterator<std::shared_ptr<Osc::Client>> clients_iter(
        this->_clients);
    while (clients_iter.hasNext()) {
        std::shared_ptr<Osc::Client> client = clients_iter.next();

        if (client->heartbeat().addSecs(this->_client_heartbeat_timeout_seconds) < QTime::currentTime()) {
            qInfo() << tr("Client %1:%2 timed out, last heartbeat was at %3")
                           .arg(client->address().toString())
                           .arg(client->port())
                           .arg(client->heartbeat().toString());
            clients_iter.remove();
        }
    }
}

std::shared_ptr<Osc::Client> Osc::Server::getClient(QHostAddress address,
    quint16 port)
{
    foreach (std::shared_ptr<Osc::Client> client, _clients) {
        if (client->address() == address) {
            if (client->port() == port) {
                return client;
            }
        }
    }

    return std::shared_ptr<Osc::Client>(nullptr);
}

void Osc::Server::sendSubscriptionUpdates()
{
    QMutableListIterator<Osc::Subscription> subscription_iterator(_subscriptions);
    while (subscription_iterator.hasNext()) {
        Osc::Subscription subscription = subscription_iterator.next();

        // TODO: Check if client is still connected

        std::shared_ptr<Alsa::Card> card = _cardmodel->card(subscription.card_index);

        if (card.get() == nullptr) {
            qWarning() << tr("Requested card index %1 is invalid, removing invalid "
                             "subscription")
                              .arg(subscription.card_index);

            subscription_iterator.remove();

            continue;
        }

        std::shared_ptr<Alsa::Ctl::Element> element = card->elementList()->getByID(subscription.element_index);

        if (element.get() == nullptr) {
            qWarning() << tr("Requested element index %1 is invalid, removing "
                             "invalid subscription")
                              .arg(subscription.element_index);

            subscription_iterator.remove();

            continue;
        }

        qDebug() << QString("Processing subscription for %1:%2/%3/%4")
                        .arg(subscription.client->address().toString())
                        .arg(subscription.client->port())
                        .arg(subscription.card_index)
                        .arg(subscription.element_index);

        sendOscMessage(createElementValueMessage(element), subscription.client);
    }
}

std::shared_ptr<Osc::Message> Osc::Server::createElementValueMessage(
    std::shared_ptr<Alsa::Ctl::Element> element)
{
    QList<QVariant> values;

    for (int i = 0; i < element->channel_count(); i++) {
        values.append(element->value(i));
    }

    return std::shared_ptr<Osc::Message>(
        new Osc::Message(QString("/card/%1/element/%2/value")
                             .arg(element->card_index())
                             .arg(element->id()),
            values));
}

void Osc::Server::endpoint_heartbeat(
    std::shared_ptr<Osc::Message> received_message)
{
    std::shared_ptr<Osc::Message> response = received_message->response();

    for (int i = 0; i < _clients.count(); i++) {
        std::shared_ptr<Osc::Client> client = _clients.at(i);

        sendOscMessage(response);

        if (client->address().isEqual(received_message->sourceAddress) && (client->port() == received_message->sourcePort)) {
            client->setHeartbeat();

            return;
        }
    }

    qInfo() << tr("New client connected at %1:%2")
                   .arg(received_message->sourceAddress.toString())
                   .arg(received_message->sourcePort);
    _clients.append(std::shared_ptr<Osc::Client>(new Osc::Client(
        received_message->sourceAddress, received_message->sourcePort)));
}

void Osc::Server::endpoint_element_subscribe(
    std::shared_ptr<Osc::Message> received_message)
{
    if (received_message->format() == "") {
        std::shared_ptr<Osc::Client> client = getClient(
            received_message->sourceAddress, received_message->sourcePort);

        if (client.get() == nullptr) {
            qWarning() << QString("Could not find client for address %1:%2")
                              .arg(received_message->sourceAddress.toString())
                              .arg(received_message->sourcePort);
            return;
        }

        // /card/0/element/0/subscribe
        QStringList address_parts = received_message->address.split('/');
        bool card_index_valid;
        int card_index = address_parts[2].toInt(&card_index_valid);

        if (!card_index_valid) {
            qWarning() << tr("Could not convert '%1' into integer for card_index")
                              .arg(address_parts[2]);
            return;
        }

        std::shared_ptr<Alsa::Card> card = _cardmodel->card(card_index);

        if (card.get() == nullptr) {
            qWarning() << tr("Requested card index %1 is invalid").arg(card_index);
            return;
        }

        bool element_index_valid;
        int element_index = address_parts[4].toInt(&element_index_valid);

        if (!element_index_valid) {
            qWarning() << tr("Could not convert '%1' into integer for element_index")
                              .arg(address_parts[4]);
            return;
        }

        std::shared_ptr<Alsa::Ctl::Element> element = card->elementList()->getByID(element_index);

        if (element.get() == nullptr) {
            qWarning()
                << tr("Requested element with id '%1' not found").arg(element_index);
            return;
        }

        std::shared_ptr<Osc::Message> response = received_message->response(QString("/card/%1/element/%2/subscribe")
                                                                                .arg(card_index)
                                                                                .arg(element_index));

        qDebug() << QString("Client %1:%2 subscribed to %3:%4 (%5)")
                        .arg(client->address().toString())
                        .arg(client->port())
                        .arg(card_index)
                        .arg(element_index)
                        .arg(element->name());
        _subscriptions.append(Subscription { client, card_index, element_index });

        sendOscMessage(response);

        return;
    }
}

void Osc::Server::endpoint_element_unsubscribe(
    std::shared_ptr<Osc::Message> received_message)
{
    if (received_message->format() == "") {
        std::shared_ptr<Osc::Client> client = getClient(
            received_message->sourceAddress, received_message->sourcePort);

        if (client.get() == nullptr) {
            qWarning() << QString("Could not find client for address %1:%2")
                              .arg(received_message->sourceAddress.toString())
                              .arg(received_message->sourcePort);
            return;
        }

        // /card/0/element/0/subscribe
        QStringList address_parts = received_message->address.split('/');
        bool card_index_valid;
        int card_index = address_parts[2].toInt(&card_index_valid);

        if (!card_index_valid) {
            qWarning() << tr("Could not convert '%1' into integer for card_index")
                              .arg(address_parts[2]);
            return;
        }

        bool element_index_valid;
        int element_index = address_parts[4].toInt(&element_index_valid);

        if (!element_index_valid) {
            qWarning() << tr("Could not convert '%1' into integer for element_index")
                              .arg(address_parts[4]);
            return;
        }

        std::shared_ptr<Osc::Message> response = received_message->response(QString("/card/%1/element/%2/unsubscribe")
                                                                                .arg(card_index)
                                                                                .arg(element_index));

        for (int i = 0; i < _subscriptions.count(); i++) {
            Osc::Subscription subscription = _subscriptions.at(i);

            if (subscription.client == client) {
                qDebug() << QString("Removing subscription for %1:%2/%3/%4")
                                .arg(client->address().toString())
                                .arg(client->port())
                                .arg(card_index)
                                .arg(element_index);

                _subscriptions.removeAt(i);
                sendOscMessage(response);

                break;
            }
        }

        return;
    }
}

void Osc::Server::endpoint_element_subscription_timer(
    std::shared_ptr<Osc::Client> client, int card_index, int element_index)
{
    std::shared_ptr<Alsa::Card> card = _cardmodel->card(card_index);

    if (card.get() == nullptr) {
        qWarning() << tr("Requested card index %1 is invalid").arg(card_index);
        return;
    }
    std::shared_ptr<Alsa::Ctl::Element> element = card->elementList()->getByID(element_index);

    if (element.get() == nullptr) {
        qWarning()
            << tr("Requested element with id '%1' not found").arg(element_index);
        return;
    }

    std::shared_ptr<Osc::Message> response(new Osc::Message(
        QString("/card/%1/element/%2/value").arg(card_index).arg(element_index),
        QList<QVariant>()));

    response->destinationAddress = client->address();
    response->destinationPort = client->port();

    qDebug() << tr("Requested element %1 has %2 channels")
                    .arg(element_index)
                    .arg(element->channel_count());

    for (int i = 0; i < element->channel_count(); i++) {
        response->values.append(element->value(i));
    }

    sendOscMessage(response);
}

void Osc::Server::endpoint_cards(
    std::shared_ptr<Osc::Message> received_message)
{
    std::shared_ptr<Osc::Message> response = received_message->response("/cards");

    foreach (int card, _cardmodel->cards()) {
        response->values.append(card);
    }

    sendOscMessage(response);
}

void Osc::Server::endpoint_card_name(
    std::shared_ptr<Osc::Message> received_message)
{
    if (received_message->format() == "") {
        // /card/0/name
        QStringList address_parts = received_message->address.split('/');
        bool card_index_valid;
        int card_index = address_parts[2].toInt(&card_index_valid);

        if (!card_index_valid) {
            qWarning()
                << tr("Could not convert '%1' into integer").arg(address_parts[2]);
            return;
        }

        std::shared_ptr<Alsa::Card> card = _cardmodel->card(card_index);

        if (card.get() == nullptr) {
            qWarning() << tr("Requested card index %1 is invalid").arg(card_index);
            return;
        }

        std::shared_ptr<Osc::Message> response = received_message->response(QString("/card/%1/name").arg(card_index));

        response->values.append(card->name());

        sendOscMessage(response);
    }
}

void Osc::Server::endpoint_card_long_name(
    std::shared_ptr<Osc::Message> received_message)
{
    if (received_message->format() == "") {
        // /card/0/longname
        QStringList address_parts = received_message->address.split('/');
        bool card_index_valid;
        int card_index = address_parts[2].toInt(&card_index_valid);

        if (!card_index_valid) {
            qWarning()
                << tr("Could not convert '%1' into integer").arg(address_parts[2]);
            return;
        }

        std::shared_ptr<Alsa::Card> card = _cardmodel->card(card_index);

        if (card.get() == nullptr) {
            qWarning() << tr("Requested card index %1 is invalid").arg(card_index);
            return;
        }

        std::shared_ptr<Osc::Message> response = received_message->response(
            QString("/card/%1/longname").arg(card_index));

        response->values.append(card->longName());

        sendOscMessage(response);
    }
}

void Osc::Server::endpoint_card_mixer_name(
    std::shared_ptr<Osc::Message> received_message)
{
    if (received_message->format() == "") {
        // /card/0/mixername
        QStringList address_parts = received_message->address.split('/');
        bool card_index_valid;
        int card_index = address_parts[2].toInt(&card_index_valid);

        if (!card_index_valid) {
            qWarning()
                << tr("Could not convert '%1' into integer").arg(address_parts[2]);
            return;
        }

        std::shared_ptr<Alsa::Card> card = _cardmodel->card(card_index);

        if (card.get() == nullptr) {
            qWarning() << tr("Requested card index %1 is invalid").arg(card_index);
            return;
        }

        std::shared_ptr<Osc::Message> response = received_message->response(
            QString("/card/%1/mixername").arg(card_index));

        response->values.append(card->mixerName());

        sendOscMessage(response);
    }
}

void Osc::Server::endpoint_card_driver(
    std::shared_ptr<Osc::Message> received_message)
{
    if (received_message->format() == "") {
        // /card/0/driver
        QStringList address_parts = received_message->address.split('/');
        bool card_index_valid;
        int card_index = address_parts[2].toInt(&card_index_valid);

        if (!card_index_valid) {
            qWarning()
                << tr("Could not convert '%1' into integer").arg(address_parts[2]);
            return;
        }

        std::shared_ptr<Alsa::Card> card = _cardmodel->card(card_index);

        if (card.get() == nullptr) {
            qWarning() << tr("Requested card index %1 is invalid").arg(card_index);
            return;
        }

        std::shared_ptr<Osc::Message> response = received_message->response(QString("/card/%1/driver").arg(card_index));

        response->values.append(card->driver());

        sendOscMessage(response);
    }
}

void Osc::Server::endpoint_card_components(
    std::shared_ptr<Osc::Message> received_message)
{
    if (received_message->format() == "") {
        // /card/0/components
        QStringList address_parts = received_message->address.split('/');
        bool card_index_valid;
        int card_index = address_parts[2].toInt(&card_index_valid);

        if (!card_index_valid) {
            qWarning()
                << tr("Could not convert '%1' into integer").arg(address_parts[2]);
            return;
        }

        std::shared_ptr<Alsa::Card> card = _cardmodel->card(card_index);

        if (card.get() == nullptr) {
            qWarning() << tr("Requested card index %1 is invalid").arg(card_index);
            return;
        }

        std::shared_ptr<Osc::Message> response = received_message->response(
            QString("/card/%1/components").arg(card_index));

        response->values.append(card->components());

        sendOscMessage(response);
    }
}

void Osc::Server::endpoint_card_id(
    std::shared_ptr<Osc::Message> received_message)
{
    if (received_message->format() == "") {
        // /card/0/id
        QStringList address_parts = received_message->address.split('/');
        bool card_index_valid;
        int card_index = address_parts[2].toInt(&card_index_valid);

        if (!card_index_valid) {
            qWarning()
                << tr("Could not convert '%1' into integer").arg(address_parts[2]);
            return;
        }

        std::shared_ptr<Alsa::Card> card = _cardmodel->card(card_index);

        if (card.get() == nullptr) {
            qWarning() << tr("Requested card index %1 is invalid").arg(card_index);
            return;
        }

        std::shared_ptr<Osc::Message> response = received_message->response(QString("/card/%1/id").arg(card_index));

        response->values.append(card->id());

        sendOscMessage(response);
    }
}

void Osc::Server::endpoint_card_sync(
    std::shared_ptr<Osc::Message> received_message)
{
    if (received_message->format() == "") {
        // /card/0/sync
        QStringList address_parts = received_message->address.split('/');
        bool card_index_valid;
        int card_index = address_parts[2].toInt(&card_index_valid);

        if (!card_index_valid) {
            qWarning()
                << tr("Could not convert '%1' into integer").arg(address_parts[2]);
            return;
        }

        std::shared_ptr<Alsa::Card> card = _cardmodel->card(card_index);

        if (card.get() == nullptr) {
            qWarning() << tr("Requested card index %1 is invalid").arg(card_index);
            return;
        }

        std::shared_ptr<Osc::Message> element_message(
            new Osc::Message("", QList<QVariant>()));
        element_message->sourceAddress = received_message->sourceAddress;
        element_message->sourcePort = received_message->sourcePort;

        for (int i = 0; i < card->elementList()->count(); i++) {
            element_message->address = QString("/card/%1/element/%2").arg(card_index).arg(i);

            endpoint_element_name(element_message);
            endpoint_element_type(element_message);
            endpoint_element_value(element_message);
            endpoint_element_minimum(element_message);
            endpoint_element_maximum(element_message);
            endpoint_element_readable(element_message);
            endpoint_element_writable(element_message);
            endpoint_element_value_dB(element_message);
            endpoint_element_minimum_dB(element_message);
            endpoint_element_maximum_dB(element_message);
            endpoint_element_enum_list(element_message);

            QThread::currentThread()->usleep(15);
        }
    }
}

void Osc::Server::endpoint_element_count(
    std::shared_ptr<Osc::Message> received_message)
{
    if (received_message->format() == "") {
        // /card/0/element/count
        QStringList address_parts = received_message->address.split('/');
        bool card_index_valid;
        int card_index = address_parts[2].toInt(&card_index_valid);

        if (!card_index_valid) {
            qWarning()
                << tr("Could not convert '%1' into integer").arg(address_parts[2]);
            return;
        }

        std::shared_ptr<Alsa::Card> card = _cardmodel->card(card_index);

        if (card.get() == nullptr) {
            qWarning() << tr("Requested card index %1 is invalid").arg(card_index);
            return;
        }

        std::shared_ptr<Osc::Message> response = received_message->response(
            QString("/card/%1/element/count").arg(card_index));

        response->values.append(card->elementList()->count());

        sendOscMessage(response);
    }
}

void Osc::Server::endpoint_element_name(
    std::shared_ptr<Osc::Message> received_message)
{
    if (received_message->format() == "") {
        // /card/0/element/0/name
        QStringList address_parts = received_message->address.split('/');
        bool card_index_valid;
        int card_index = address_parts[2].toInt(&card_index_valid);

        if (!card_index_valid) {
            qWarning() << tr("Could not convert '%1' into integer for card_index")
                              .arg(address_parts[2]);
            return;
        }

        std::shared_ptr<Alsa::Card> card = _cardmodel->card(card_index);

        if (card.get() == nullptr) {
            qWarning() << tr("Requested card index %1 is invalid").arg(card_index);
            return;
        }

        bool element_index_valid;
        int element_index = address_parts[4].toInt(&element_index_valid);

        if (!element_index_valid) {
            qWarning() << tr("Could not convert '%1' into integer for element_index")
                              .arg(address_parts[4]);
            return;
        }

        std::shared_ptr<Alsa::Ctl::Element> element = card->elementList()->getByID(element_index);

        if (element.get() == nullptr) {
            qWarning()
                << tr("Requested element with id '%1' not found").arg(element_index);
            return;
        }

        std::shared_ptr<Osc::Message> response = received_message->response(
            QString("/card/%1/element/%2/name").arg(card_index).arg(element_index));

        response->values.append(element->name());

        sendOscMessage(response);
    }
}

void Osc::Server::endpoint_element_type(
    std::shared_ptr<Osc::Message> received_message)
{

    if (received_message->format() == "") {
        // /card/0/element/0/type
        QStringList address_parts = received_message->address.split('/');
        bool card_index_valid;
        int card_index = address_parts[2].toInt(&card_index_valid);

        if (!card_index_valid) {
            qWarning() << tr("Could not convert '%1' into integer for card_index")
                              .arg(address_parts[2]);
            return;
        }

        std::shared_ptr<Alsa::Card> card = _cardmodel->card(card_index);

        if (card.get() == nullptr) {
            qWarning() << tr("Requested card index %1 is invalid").arg(card_index);
            return;
        }

        bool element_index_valid;
        int element_index = address_parts[4].toInt(&element_index_valid);

        if (!element_index_valid) {
            qWarning() << tr("Could not convert '%1' into integer for element_index")
                              .arg(address_parts[4]);
            return;
        }

        std::shared_ptr<Alsa::Ctl::Element> element = card->elementList()->getByID(element_index);

        if (element.get() == nullptr) {
            qWarning()
                << tr("Requested element with id '%1' not found").arg(element_index);
            return;
        }

        std::shared_ptr<Osc::Message> response = received_message->response(
            QString("/card/%1/element/%2/type").arg(card_index).arg(element_index));

        response->values.append(element->type());

        sendOscMessage(response);
    }
}

void Osc::Server::endpoint_element_value(
    std::shared_ptr<Osc::Message> received_message)
{
    if (received_message->format() == "") {
        // /card/0/element/0/value
        QStringList address_parts = received_message->address.split('/');
        bool card_index_valid;
        int card_index = address_parts[2].toInt(&card_index_valid);

        if (!card_index_valid) {
            qWarning() << tr("Could not convert '%1' into integer for card_index")
                              .arg(address_parts[2]);
            return;
        }

        std::shared_ptr<Alsa::Card> card = _cardmodel->card(card_index);

        if (card.get() == nullptr) {
            qWarning() << tr("Requested card index %1 is invalid").arg(card_index);
            return;
        }

        bool element_index_valid;
        int element_index = address_parts[4].toInt(&element_index_valid);

        if (!element_index_valid) {
            qWarning() << tr("Could not convert '%1' into integer for element_index")
                              .arg(address_parts[4]);
            return;
        }

        std::shared_ptr<Alsa::Ctl::Element> element = card->elementList()->getByID(element_index);

        if (element.get() == nullptr) {
            qWarning()
                << tr("Requested element with id '%1' not found").arg(element_index);
            return;
        }

        std::shared_ptr<Osc::Message> response = received_message->response(QString("/card/%1/element/%2/value")
                                                                                .arg(card_index)
                                                                                .arg(element_index));

        qDebug() << tr("Requested element %1 has %2 channels")
                        .arg(element_index)
                        .arg(element->channel_count());

        for (int i = 0; i < element->channel_count(); i++) {
            response->values.append(element->value(i));
        }

        sendOscMessage(response);
    } else if (received_message->format() == "i") {
        // /card/0/element/0/value
        QStringList address_parts = received_message->address.split('/');
        bool card_index_valid;
        int card_index = address_parts[2].toInt(&card_index_valid);

        if (!card_index_valid) {
            qWarning() << tr("Could not convert '%1' into integer for card_index")
                              .arg(address_parts[2]);
            return;
        }

        std::shared_ptr<Alsa::Card> card = _cardmodel->card(card_index);

        if (card.get() == nullptr) {
            qWarning() << tr("Requested card index %1 is invalid").arg(card_index);
            return;
        }

        bool element_index_valid;
        int element_index = address_parts[4].toInt(&element_index_valid);

        if (!element_index_valid) {
            qWarning() << tr("Could not convert '%1' into integer for element_index")
                              .arg(address_parts[4]);
            return;
        }

        std::shared_ptr<Alsa::Ctl::Element> element = card->elementList()->getByID(element_index);

        if (element.get() == nullptr) {
            qWarning()
                << tr("Requested element with id '%1' not found").arg(element_index);
            return;
        }

        std::shared_ptr<Osc::Message> response = received_message->response(QString("/card/%1/element/%2/value")
                                                                                .arg(card_index)
                                                                                .arg(element_index));

        qDebug() << tr("Requested element %1 has %2 channels")
                        .arg(element_index)
                        .arg(element->channel_count());

        for (int i = 0; i < element->channel_count(); i++) {
            element->setValue(received_message->values[0], i);
        }

        sendOscMessage(response);
    }
}

void Osc::Server::endpoint_element_minimum(
    std::shared_ptr<Osc::Message> received_message)
{
    if (received_message->format() == "") {
        // /card/0/element/0/minimum
        QStringList address_parts = received_message->address.split('/');
        bool card_index_valid;
        int card_index = address_parts[2].toInt(&card_index_valid);

        if (!card_index_valid) {
            qWarning() << tr("Could not convert '%1' into integer for card_index")
                              .arg(address_parts[2]);
            return;
        }

        std::shared_ptr<Alsa::Card> card = _cardmodel->card(card_index);

        if (card.get() == nullptr) {
            qWarning() << tr("Requested card index %1 is invalid").arg(card_index);
            return;
        }

        bool element_index_valid;
        int element_index = address_parts[4].toInt(&element_index_valid);

        if (!element_index_valid) {
            qWarning() << tr("Could not convert '%1' into integer for element_index")
                              .arg(address_parts[4]);
            return;
        }

        std::shared_ptr<Alsa::Ctl::Element> element = card->elementList()->getByID(element_index);

        if (element.get() == nullptr) {
            qWarning()
                << tr("Requested element with id '%1' not found").arg(element_index);
            return;
        }

        std::shared_ptr<Osc::Message> response = received_message->response(QString("/card/%1/element/%2/minimum")
                                                                                .arg(card_index)
                                                                                .arg(element_index));

        response->values.append(element->min());

        sendOscMessage(response);
    }
}

void Osc::Server::endpoint_element_maximum(
    std::shared_ptr<Osc::Message> received_message)
{
    if (received_message->format() == "") {
        // /card/0/element/0/maximum
        QStringList address_parts = received_message->address.split('/');
        bool card_index_valid;
        int card_index = address_parts[2].toInt(&card_index_valid);

        if (!card_index_valid) {
            qWarning() << tr("Could not convert '%1' into integer for card_index")
                              .arg(address_parts[2]);
            return;
        }

        std::shared_ptr<Alsa::Card> card = _cardmodel->card(card_index);

        if (card.get() == nullptr) {
            qWarning() << tr("Requested card index %1 is invalid").arg(card_index);
            return;
        }

        bool element_index_valid;
        int element_index = address_parts[4].toInt(&element_index_valid);

        if (!element_index_valid) {
            qWarning() << tr("Could not convert '%1' into integer for element_index")
                              .arg(address_parts[4]);
            return;
        }

        std::shared_ptr<Alsa::Ctl::Element> element = card->elementList()->getByID(element_index);

        if (element.get() == nullptr) {
            qWarning()
                << tr("Requested element with id '%1' not found").arg(element_index);
            return;
        }

        std::shared_ptr<Osc::Message> response = received_message->response(QString("/card/%1/element/%2/maximum")
                                                                                .arg(card_index)
                                                                                .arg(element_index));

        response->values.append(element->max());

        sendOscMessage(response);
    }
}

void Osc::Server::endpoint_element_readable(
    std::shared_ptr<Osc::Message> received_message)
{
    if (received_message->format() == "") {
        // /card/0/element/0/readable
        QStringList address_parts = received_message->address.split('/');
        bool card_index_valid;
        int card_index = address_parts[2].toInt(&card_index_valid);

        if (!card_index_valid) {
            qWarning() << tr("Could not convert '%1' into integer for card_index")
                              .arg(address_parts[2]);
            return;
        }

        std::shared_ptr<Alsa::Card> card = _cardmodel->card(card_index);

        if (card.get() == nullptr) {
            qWarning() << tr("Requested card index %1 is invalid").arg(card_index);
            return;
        }

        bool element_index_valid;
        int element_index = address_parts[4].toInt(&element_index_valid);

        if (!element_index_valid) {
            qWarning() << tr("Could not convert '%1' into integer for element_index")
                              .arg(address_parts[4]);
            return;
        }

        std::shared_ptr<Alsa::Ctl::Element> element = card->elementList()->getByID(element_index);

        if (element.get() == nullptr) {
            qWarning()
                << tr("Requested element with id '%1' not found").arg(element_index);
            return;
        }

        std::shared_ptr<Osc::Message> response = received_message->response(QString("/card/%1/element/%2/readable")
                                                                                .arg(card_index)
                                                                                .arg(element_index));

        response->values.append(element->isReadable());

        sendOscMessage(response);
    }
}

void Osc::Server::endpoint_element_writable(
    std::shared_ptr<Osc::Message> received_message)
{
    if (received_message->format() == "") {
        // /card/0/element/0/writable
        QStringList address_parts = received_message->address.split('/');
        bool card_index_valid;
        int card_index = address_parts[2].toInt(&card_index_valid);

        if (!card_index_valid) {
            qWarning() << tr("Could not convert '%1' into integer for card_index")
                              .arg(address_parts[2]);
            return;
        }

        std::shared_ptr<Alsa::Card> card = _cardmodel->card(card_index);

        if (card.get() == nullptr) {
            qWarning() << tr("Requested card index %1 is invalid").arg(card_index);
            return;
        }

        bool element_index_valid;
        int element_index = address_parts[4].toInt(&element_index_valid);

        if (!element_index_valid) {
            qWarning() << tr("Could not convert '%1' into integer for element_index")
                              .arg(address_parts[4]);
            return;
        }

        std::shared_ptr<Alsa::Ctl::Element> element = card->elementList()->getByID(element_index);

        if (element.get() == nullptr) {
            qWarning()
                << tr("Requested element with id '%1' not found").arg(element_index);
            return;
        }

        std::shared_ptr<Osc::Message> response = received_message->response(QString("/card/%1/element/%2/writable")
                                                                                .arg(card_index)
                                                                                .arg(element_index));

        response->values.append(element->isWritable());

        sendOscMessage(response);
    }
}

void Osc::Server::endpoint_element_value_dB(
    std::shared_ptr<Osc::Message> received_message)
{
    if (received_message->format() == "") {
        // /card/0/element/0/value
        QStringList address_parts = received_message->address.split('/');
        bool card_index_valid;
        int card_index = address_parts[2].toInt(&card_index_valid);

        if (!card_index_valid) {
            qWarning() << tr("Could not convert '%1' into integer for card_index")
                              .arg(address_parts[2]);
            return;
        }

        std::shared_ptr<Alsa::Card> card = _cardmodel->card(card_index);

        if (card.get() == nullptr) {
            qWarning() << tr("Requested card index %1 is invalid").arg(card_index);
            return;
        }

        bool element_index_valid;
        int element_index = address_parts[4].toInt(&element_index_valid);

        if (!element_index_valid) {
            qWarning() << tr("Could not convert '%1' into integer for element_index")
                              .arg(address_parts[4]);
            return;
        }

        std::shared_ptr<Alsa::Ctl::Element> element = card->elementList()->getByID(element_index);

        if (element.get() == nullptr) {
            qWarning()
                << tr("Requested element with id '%1' not found").arg(element_index);
            return;
        }

        std::shared_ptr<Osc::Message> response = received_message->response(QString("/card/%1/element/%2/db/value")
                                                                                .arg(card_index)
                                                                                .arg(element_index));

        qDebug() << tr("Requested element %1 has %2 channels")
                        .arg(element_index)
                        .arg(element->channel_count());

        for (int i = 0; i < element->channel_count(); i++) {
            response->values.append(element->valuedB(i));
        }

        sendOscMessage(response);
    } else if (received_message->format() == "i") {
        if (received_message->format() == "") {
            // /card/0/element/0/db/value
            QStringList address_parts = received_message->address.split('/');
            bool card_index_valid;
            int card_index = address_parts[2].toInt(&card_index_valid);

            if (!card_index_valid) {
                qWarning() << tr("Could not convert '%1' into integer for card_index")
                                  .arg(address_parts[2]);
                return;
            }

            std::shared_ptr<Alsa::Card> card = _cardmodel->card(card_index);

            if (card.get() == nullptr) {
                qWarning() << tr("Requested card index %1 is invalid").arg(card_index);
                return;
            }

            bool element_index_valid;
            int element_index = address_parts[4].toInt(&element_index_valid);

            if (!element_index_valid) {
                qWarning()
                    << tr("Could not convert '%1' into integer for element_index")
                           .arg(address_parts[4]);
                return;
            }

            std::shared_ptr<Alsa::Ctl::Element> element = card->elementList()->getByID(element_index);

            if (element.get() == nullptr) {
                qWarning() << tr("Requested element with id '%1' not found")
                                  .arg(element_index);
                return;
            }

            std::shared_ptr<Osc::Message> response = received_message->response(QString("/card/%1/element/%2/db/value")
                                                                                    .arg(card_index)
                                                                                    .arg(element_index));

            qDebug() << tr("Requested element %1 has %2 channels")
                            .arg(element_index)
                            .arg(element->channel_count());

            for (int i = 0; i < element->channel_count(); i++) {
                element->setValuedB(received_message->values[0], i);
            }

            sendOscMessage(response);
        }
    }
}

void Osc::Server::endpoint_element_minimum_dB(
    std::shared_ptr<Osc::Message> received_message)
{
    if (received_message->format() == "") {
        // /card/0/element/0/db/minimum
        QStringList address_parts = received_message->address.split('/');
        bool card_index_valid;
        int card_index = address_parts[2].toInt(&card_index_valid);

        if (!card_index_valid) {
            qWarning() << tr("Could not convert '%1' into integer for card_index")
                              .arg(address_parts[2]);
            return;
        }

        std::shared_ptr<Alsa::Card> card = _cardmodel->card(card_index);

        if (card.get() == nullptr) {
            qWarning() << tr("Requested card index %1 is invalid").arg(card_index);
            return;
        }

        bool element_index_valid;
        int element_index = address_parts[4].toInt(&element_index_valid);

        if (!element_index_valid) {
            qWarning() << tr("Could not convert '%1' into integer for element_index")
                              .arg(address_parts[4]);
            return;
        }

        std::shared_ptr<Alsa::Ctl::Element> element = card->elementList()->getByID(element_index);

        if (element.get() == nullptr) {
            qWarning()
                << tr("Requested element with id '%1' not found").arg(element_index);
            return;
        }

        std::shared_ptr<Osc::Message> response = received_message->response(QString("/card/%1/element/%2/db/minimum")
                                                                                .arg(card_index)
                                                                                .arg(element_index));

        response->values.append(element->mindB());

        sendOscMessage(response);
    }
}

void Osc::Server::endpoint_element_maximum_dB(
    std::shared_ptr<Osc::Message> received_message)
{
    if (received_message->format() == "") {
        // /card/0/element/0/db/maximum
        QStringList address_parts = received_message->address.split('/');
        bool card_index_valid;
        int card_index = address_parts[2].toInt(&card_index_valid);

        if (!card_index_valid) {
            qWarning() << tr("Could not convert '%1' into integer for card_index")
                              .arg(address_parts[2]);
            return;
        }

        std::shared_ptr<Alsa::Card> card = _cardmodel->card(card_index);

        if (card.get() == nullptr) {
            qWarning() << tr("Requested card index %1 is invalid").arg(card_index);
            return;
        }

        bool element_index_valid;
        int element_index = address_parts[4].toInt(&element_index_valid);

        if (!element_index_valid) {
            qWarning() << tr("Could not convert '%1' into integer for element_index")
                              .arg(address_parts[4]);
            return;
        }

        std::shared_ptr<Alsa::Ctl::Element> element = card->elementList()->getByID(element_index);

        if (element.get() == nullptr) {
            qWarning()
                << tr("Requested element with id '%1' not found").arg(element_index);
            return;
        }

        std::shared_ptr<Osc::Message> response = received_message->response(QString("/card/%1/element/%2/db/maximum")
                                                                                .arg(card_index)
                                                                                .arg(element_index));

        response->values.append(element->maxdB());

        sendOscMessage(response);
    }
}

void Osc::Server::endpoint_element_enum_list(
    std::shared_ptr<Osc::Message> received_message)
{
    if (received_message->format() == "") {
        // /card/0/element/0/enum/list
        QStringList address_parts = received_message->address.split('/');
        bool card_index_valid;
        int card_index = address_parts[2].toInt(&card_index_valid);

        if (!card_index_valid) {
            qWarning() << tr("Could not convert '%1' into integer for card_index")
                              .arg(address_parts[2]);
            return;
        }

        std::shared_ptr<Alsa::Card> card = _cardmodel->card(card_index);

        if (card.get() == nullptr) {
            qWarning() << tr("Requested card index %1 is invalid").arg(card_index);
            return;
        }

        bool element_index_valid;
        int element_index = address_parts[4].toInt(&element_index_valid);

        if (!element_index_valid) {
            qWarning() << tr("Could not convert '%1' into integer for element_index")
                              .arg(address_parts[4]);
            return;
        }

        std::shared_ptr<Alsa::Ctl::Element> element = card->elementList()->getByID(element_index);

        if (element.get() == nullptr) {
            qWarning()
                << tr("Requested element with id '%1' not found").arg(element_index);
            return;
        }

        std::shared_ptr<Osc::Message> response = received_message->response(QString("/card/%1/element/%2/enum/list")
                                                                                .arg(card_index)
                                                                                .arg(element_index));

        foreach (QString value, element->enum_list()) {
            response->values.append(value);
        }
        sendOscMessage(response);
    }
}

void Osc::Server::endpoint_element_enum_name(
    std::shared_ptr<Osc::Message> received_message)
{
    if (received_message->format() == "") {
        // /card/0/element/0/enum/0/name
        QStringList address_parts = received_message->address.split('/');
        bool card_index_valid;
        int card_index = address_parts[2].toInt(&card_index_valid);

        if (!card_index_valid) {
            qWarning() << tr("Could not convert '%1' into integer for card_index")
                              .arg(address_parts[2]);
            return;
        }

        std::shared_ptr<Alsa::Card> card = _cardmodel->card(card_index);

        if (card.get() == nullptr) {
            qWarning() << tr("Requested card index %1 is invalid").arg(card_index);
            return;
        }

        bool element_index_valid;
        int element_index = address_parts[4].toInt(&element_index_valid);

        if (!element_index_valid) {
            qWarning() << tr("Could not convert '%1' into integer for element_index")
                              .arg(address_parts[4]);
            return;
        }

        std::shared_ptr<Alsa::Ctl::Element> element = card->elementList()->getByID(element_index);

        if (element.get() == nullptr) {
            qWarning()
                << tr("Requested element with id '%1' not found").arg(element_index);
            return;
        }

        if (element->type() != 3) {
            qWarning()
                << tr("Element %1:%2 is no enum").arg(card_index).arg(element_index);
            return;
        }

        bool enum_index_valid;
        int enum_index = address_parts[6].toInt(&enum_index_valid);

        if (!enum_index_valid) {
            qWarning() << tr("Could not convert '%1' into integer for element_index")
                              .arg(address_parts[6]);
            return;
        }

        std::shared_ptr<Osc::Message> response = received_message->response(
            QString("/card/%1/element/%2/name").arg(card_index).arg(element_index));

        response->values.append(element->enum_name(enum_index));

        sendOscMessage(response);
    }
}

void Osc::Server::endpoint_element_by_name(
    std::shared_ptr<Osc::Message> received_message)
{
    if (received_message->format() == "s") {
        // /card/0/element/by-name
        QStringList address_parts = received_message->address.split('/');
        bool card_index_valid;
        int card_index = address_parts[2].toInt(&card_index_valid);

        if (!card_index_valid) {
            qWarning() << tr("Could not convert '%1' into integer for card_index")
                              .arg(address_parts[2]);
            return;
        }

        std::shared_ptr<Alsa::Card> card = _cardmodel->card(card_index);

        if (card.get() == nullptr) {
            qWarning() << tr("Requested card index %1 is invalid").arg(card_index);
            return;
        }

        QString element_name = received_message->values[0].toString();

        std::shared_ptr<Alsa::Ctl::Element> element = card->elementList()->getByName(element_name);

        if (element.get() == nullptr) {
            qWarning()
                << tr("Requested element with name '%1' not found").arg(element_name);
            return;
        }

        std::shared_ptr<Osc::Message> response = received_message->response(
            QString("/card/%1/element/by-name").arg(card_index));

        response->values.append(element_name);

        response->values.append(element->id());

        sendOscMessage(response);
    }
}
