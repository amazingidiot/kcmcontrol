#pragma once

#include <QByteArray>
#include <QHostAddress>
#include <QList>
#include <QNetworkDatagram>
#include <QVariant>
#include <memory>

namespace Osc {
class Message {
public:
    Message();
    Message(QHostAddress destinationAddress, quint16 destinationPort,
        QHostAddress sourceAddress, quint16 sourcePort, QString address,
        QList<QVariant> values);
    Message(QString address, QList<QVariant> values);

    Osc::Message response();
    Osc::Message response(QString address);

    QHostAddress sourceAddress;
    quint16 sourcePort;

    QHostAddress destinationAddress;
    quint16 destinationPort;

    QString address;

    QString format();
    QVariantList values;

    QByteArray toByteArray();

    QString toString();
};
} // namespace Osc
