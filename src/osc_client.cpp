#include "osc_client.h"
#include "client_manager.h"
#include <QDnsLookup>
#include <QDnsServiceRecord>
#include <QObject>

Osc::Client::Client(::Client::Manager* parent, QTcpSocket* socket, QString hostname)
    : QObject{parent}
    , _hostname(hostname)
    , _manager(parent)
{
    initializeSocket(_socket);

    if (_hostname != "") {
        qInfo().noquote() << QString("New client: %1 (%2:%3)")
                                 .arg(_hostname)
                                 .arg(_socket->peerAddress().toString())
                                 .arg(_socket->peerPort());
    } else {
        qInfo().noquote() << QString("New client: %2:%3")
                                 .arg(_socket->peerAddress().toString())
                                 .arg(_socket->peerPort());
    }
}

Osc::Client::Client(::Client::Manager* parent, QString hostname, quint16 port)
    : _hostname(hostname)
    , _port(port)
    , _manager(parent)
{
    qDebug().noquote() << QString("Connecting to %1:%2").arg(_hostname).arg(_port);

    initializeSocket(new QTcpSocket(this));

    _dnsLookup = new QDnsLookup(this);

    _dnsLookup->setType(QDnsLookup::A);
    _dnsLookup->setName(_hostname);

    connect(_dnsLookup, &QDnsLookup::finished, this, &Client::onLookupFinished);

    _dnsLookup->lookup();
}

Osc::Client::~Client()
{
    if (_hostname != "") {
        qInfo() << QString("Destroying client: %1 (%2:%3)").arg(_hostname).arg(_socket->peerAddress().toString()).arg(_socket->peerPort());
    } else {
        qInfo() << QString("Destroying client: %2:%3").arg(_socket->peerAddress().toString()).arg(_socket->peerPort());
    }
}

QHostAddress Osc::Client::peerAddress()
{
    if (_socket != nullptr) {
        return _socket->peerAddress();
    } else {
        return QHostAddress();
    }
}

quint16 Osc::Client::peerPort()
{
    if (_socket != nullptr) {
        return _socket->peerPort();
    } else {
        return 0;
    }
}

QHostAddress Osc::Client::localAddress()
{
    if (_socket != nullptr) {
        return _socket->localAddress();

    } else {
        return QHostAddress();
    }
}

quint16 Osc::Client::localPort()
{
    if (_socket != nullptr) {
        return _socket->localPort();
    } else {
        return 0;
    }
}

void Osc::Client::initializeSocket(QTcpSocket* socket)
{
    _socket = socket;

    connect(_socket, &QTcpSocket::connected, this, &Client::onSocketConnected);
    connect(_socket, &QTcpSocket::disconnected, this, &Client::onSocketDisconnected);
    connect(_socket, &QIODevice::readyRead, this, &Client::onReadyRead);

    connect(this, &::Osc::Client::disconnected, _manager, &::Client::Manager::removeClient);

    _socketStream.setDevice(_socket);
}

void Osc::Client::onSocketDisconnected()
{
    qDebug() << "Connection closed from" << _socket->peerAddress().toString()
             << _socket->peerPort();

    emit disconnected(this);
}

void Osc::Client::onReadyRead()
{
    qDebug() << "Reading" << _socket->bytesAvailable() << "bytes";

    QByteArray buffer;

    _socketStream >> buffer;

    Osc::Message message;

    message.sourceAddress = _socket->peerAddress();
    message.sourcePort = _socket->peerPort();
    message.destinationAddress = _socket->localAddress();
    message.destinationPort = _socket->localPort();

    message.address = QString(buffer.constData());

    qsizetype format_start = buffer.indexOf(',');
    qsizetype format_end = buffer.indexOf('\0', format_start);

    qsizetype value_marker = format_end + 4 - format_end % 4;

    for (int i = format_start + 1; i < format_end; i++) {
        char currentFormat = buffer.at(i);

        switch (currentFormat) {
        case 'i': {
            qint32 value = 0;

            QByteArray slice = buffer.sliced(value_marker, sizeof(value));

            QDataStream(slice) >> value;

            message.values.append(value);

            value_marker += sizeof(value);
        } break;
        case 'f': {
            float value = 0.0f;

            QByteArray slice = buffer.sliced(value_marker, sizeof(value));

            QDataStream stream(slice);
            stream.setFloatingPointPrecision(QDataStream::SinglePrecision);

            stream >> value;

            message.values.append(value);

            value_marker += sizeof(value);
        } break;
        case 's': {
            qsizetype string_length = buffer.indexOf('\0', value_marker) - value_marker;

            QByteArray slice = buffer.sliced(value_marker, string_length);

            QString value(slice);

            message.values.append(value);

            value_marker = value_marker + string_length + 4 - string_length % 4;
        } break;
        case 'b': {
            qint32 blob_size = 0;

            QByteArray slice = buffer.sliced(value_marker, sizeof(blob_size));

            QDataStream(slice) >> blob_size;

            QByteArray blob = buffer.sliced(value_marker + sizeof(blob_size), blob_size);

            qDebug() << "blob_size:" << blob_size;

            message.values.append(blob);

            value_marker = value_marker + blob_size + 4 - blob_size % 4;
        } break;
        case 'T': {
            message.values.append(true);
        } break;
        case 'F': {
            message.values.append(false);
        } break;
        case 'N': {
            message.values.append(QChar::Null);
        } break;
        }
    }

    /*
     * TODO: Check if data is left in buffer
     * TODO: Implement transaction logic or something for incomplete packages
     * TODO: Make sure enough data is available when reading from buffer
     */

    emit received(message);
}

void Osc::Client::onSocketConnected()
{
    qDebug().noquote() << QString("Connected to %1:%2").arg(peerAddress().toString()).arg(peerPort());
}

void Osc::Client::onLookupFinished()
{
    // Process clients here
    qDebug().noquote() << QString("Processing lookup results for %1").arg(_hostname);

    if (_dnsLookup->hostAddressRecords().length() == 0) {
        qWarning().noquote() << QString("No lookup results for %1").arg(_hostname);
        this->deleteLater();

        return;
    }

    QDnsHostAddressRecord record = _dnsLookup->hostAddressRecords().at(0);

    qDebug().noquote() << QString("%1 -> %2").arg(record.name(), record.value().toString());

    _socket->connectToHost(QHostAddress(record.value()), _port);
}
