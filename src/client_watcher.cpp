#include "client_watcher.h"

#include <QDebug>

namespace Client {
Watcher::Watcher(QString serviceName, QObject* parent)
    : QObject { parent }
{
    qDebug() << "Starting watcher for service" << serviceName;

    _dnsLookup = new QDnsLookup(this);

    _dnsLookup->setType(QDnsLookup::SRV);
    _dnsLookup->setName("_osc._tcp.vikey.local");

    connect(_dnsLookup, &QDnsLookup::finished, this, &Client::Watcher::lookupFinished);

    _updateTimer = new QTimer(this);

    _updateTimer->setInterval(5000);

    connect(_updateTimer, &QTimer::timeout, this, &Client::Watcher::updateClients);

    updateClients();
}

Watcher::~Watcher()
{
    qDebug() << "Destroying watcher";
}

quint32 Watcher::updateInterval() const
{
    return _updateTimer->interval();
}

void Watcher::setUpdateInterval(quint32 newUpdateInterval)
{
    _updateTimer->setInterval(newUpdateInterval);
}

void Watcher::updateClients()
{
    qDebug() << "Updating clients";

    _dnsLookup->lookup();
}

void Watcher::lookupFinished()
{
    // Process clients here
    qDebug() << "Processing lookup results";

    foreach (QDnsServiceRecord record, _dnsLookup->serviceRecords()) {
        qDebug() << QString("%1:%2 %3 -> %4")
                        .arg(record.name())
                        .arg(record.port())
                        .arg(record.priority())
                        .arg(record.target());
    }
}
}
