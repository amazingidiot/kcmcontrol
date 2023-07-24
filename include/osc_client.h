#pragma once

#include "osc_message.h"
#include <QDnsLookup>
#include <QDnsServiceRecord>
#include <QTcpSocket>

namespace Client {
class Manager;
}

namespace Osc {

class Client : public QObject {
    Q_OBJECT
public:
    Client(::Client::Manager* parent, QTcpSocket* socket, QString hostname = "");
    Client(::Client::Manager* parent, QString hostname, quint16 port);

    ~Client();

    QHostAddress peerAddress();
    quint16 peerPort();

    QHostAddress localAddress();
    quint16 localPort();

signals:
    void connected();
    void disconnected(Osc::Client* client);
    void connectionFailed();
    void received(Osc::Message message);

private:
    QTcpSocket* _socket;
    QDataStream _socketStream;
    QDnsLookup* _dnsLookup;

    QString _hostname;
    quint16 _port;

    ::Client::Manager* _manager;

    void initializeSocket(QTcpSocket* socket);

private slots:
    void onSocketDisconnected();
    void onReadyRead();
    void onSocketConnected();
    void onLookupFinished();
};
} // namespace Osc
