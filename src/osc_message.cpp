#include "osc_message.h"

Osc::Message::Message() { }

Osc::Message::Message(QHostAddress destinationAddress, quint16 destinationPort,
    QHostAddress sourceAddress, quint16 sourcePort,
    QString address, QList<QVariant> values)
{
    this->destinationAddress = destinationAddress;
    this->destinationPort = destinationPort;
    this->sourceAddress = sourceAddress;
    this->sourcePort = sourcePort;

    this->address = address;
    this->values = values;
}

Osc::Message::Message(QString address, QList<QVariant> values)
{
    this->address = address;
    this->values = values;
}

Osc::Message Osc::Message::response()
{
    Osc::Message response(
        this->sourceAddress, this->sourcePort, this->destinationAddress,
        this->destinationPort, this->address, QList<QVariant>());
    return response;
}

Osc::Message Osc::Message::response(QString address)
{
    Osc::Message response = this->response();

    response.address = address;

    return response;
}

QString Osc::Message::format()
{
    QVariant element;
    QString format;

    foreach (element, values) {
        switch (element.userType()) {
        case (QMetaType::UInt):
        case (QMetaType::Int): {
            format.append('i');
        } break;
        case (QMetaType::Bool): {
            format.append(element.toBool() ? 'T' : 'F');
        } break;
        case (QMetaType::Float): {
            format.append('f');
        } break;
        case (QMetaType::QString): {
            format.append('s');
        } break;
        case (QMetaType::QByteArray): {
            format.append('b');
        } break;
        }
    }

    return format;
}

QByteArray Osc::Message::toByteArray()
{
    QByteArray data;
    {
        QByteArray address = this->address.toLatin1();
        address.append(QChar::Null);

        while (address.length() % 4 > 0) {
            address.append(QChar::Null);
        }

        data.append(address);
    }
    {
        QByteArray format;

        format.append(',');
        format.append(this->format().toLatin1());
        format.append(QChar::Null);

        while (format.length() % 4 > 0) {
            format.append(QChar::Null);
        };

        data.append(format);
    }

    QVariant element;

    foreach (element, values) {
        switch (element.userType()) {
        case (QMetaType::UInt): {
            QByteArray uint_value;
            QDataStream stream(&uint_value, QIODevice::WriteOnly);
            stream << element.toUInt();
            data.append(uint_value);
        } break;
        case (QMetaType::Int): {
            QByteArray int_value;
            QDataStream stream(&int_value, QIODevice::WriteOnly);
            stream << element.toInt();
            data.append(int_value);
        } break;
        case (QMetaType::Float): {
            QByteArray float_value;
            QDataStream stream(&float_value, QIODevice::WriteOnly);
            stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
            stream << element.toFloat();
            data.append(float_value);
        } break;
        case (QMetaType::QString): {
            QByteArray string_value = element.toString().toLatin1();
            string_value.append(QChar::Null);

            while (string_value.length() % 4 > 0) {
                string_value.append(QChar::Null);
            };

            data.append(string_value);
        } break;
        case (QMetaType::QByteArray): {
            QByteArray blob_value = element.toByteArray();
            blob_value.append(QChar::Null);

            while (blob_value.length() % 4 > 0) {
                blob_value.append(QChar::Null);
            };

            data.append(blob_value);
        } break;
        }
    }

    return data;
}

QString Osc::Message::toString()
{
    QString valueString = "";

    foreach (QVariant value, this->values) {
        if (value.typeId() == QMetaType::Int) {
            valueString += QString("\ni(%1)").arg(value.toInt());
            continue;
        }
        if (value.typeId() == QMetaType::Bool) {
            valueString += QString("\nb(%1)").arg(value.toBool());
            continue;
        }
        if (value.typeId() == QMetaType::Float) {
            valueString += QString("\nf(%1)").arg(value.toFloat());
            continue;
        }
        if (value.typeId() == QMetaType::QString) {
            valueString += QString("\ns(%1)").arg(value.toString());
            continue;
        }
    }

    return QAbstractSocket::tr(
        "Sender: %1:%2, Receiver: %3:%4, Address: %5, Values: %6")
        .arg(this->sourceAddress.toString())
        .arg(this->sourcePort)
        .arg(this->destinationAddress.toString())
        .arg(this->destinationPort)
        .arg(this->address)
        .arg(valueString);
}
