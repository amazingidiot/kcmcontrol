#include "osc_client.h"

#include <QDnsLookup>
#include <QDnsServiceRecord>
#include <QObject>

Osc::Client::Client(QTcpSocket* socket, QString hostname)
{
    assert(socket != nullptr);

    _hostname = hostname;
    _socket = socket;

    connect(_socket, &QTcpSocket::disconnected, this, &Client::socketDisconnected);

    if (_hostname != "") {
        qInfo() << QString("New client: %1 (%2:%3)").arg(_hostname).arg(_socket->peerAddress().toString()).arg(_socket->peerPort());
    } else {
        qInfo() << QString("New client: %2:%3").arg(_socket->peerAddress().toString()).arg(_socket->peerPort());
    }
}

Osc::Client::~Client()
{
    if (_hostname != "") {
        qInfo() << QString("Destroying client: %1 (%2:%3)").arg(_hostname).arg(_socket->peerAddress().toString()).arg(_socket->peerPort());
    } else {
        qInfo() << QString("Destroying client: %2:%3").arg(_socket->peerAddress().toString()).arg(_socket->peerPort());
    }
}

void Osc::Client::socketDisconnected()
{
    qDebug() << "Connection closed from" << _socket->peerAddress().toString()
             << _socket->peerPort();

    emit disconnected(this);
}
