#pragma once

#include "osc_message.h"

#include <QDnsLookup>
#include <QDnsServiceRecord>
#include <QTcpSocket>

#include <memory>

namespace Osc {

class Client : public QObject {
    Q_OBJECT
public:
    Client(QTcpSocket *socket, QString hostname = "");

    ~Client();

signals:
    void connected();
    void disconnected(Osc::Client *client);
    void connectionFailed();
    void received(Osc::Message message);

private:
    QTcpSocket* _socket;
    QString _hostname;

private slots:
    void onSocketDisconnected();
    void onReadyRead();
};
} // namespace Osc
