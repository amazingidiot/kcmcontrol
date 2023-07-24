#include "client_watcher.h"
#include "client_manager.h"
#include <QDebug>

namespace Client {
Watcher::Watcher(Manager* parent)
    : QObject { parent }
{
    qDebug() << "Creating watcher";

    _manager = parent;
}

Watcher::~Watcher()
{
    qDebug() << "Destroying watcher";
}

int Watcher::init(QString serviceName)
{
    qDebug() << "Starting watcher for service" << serviceName;

    QString dnsServiceName = QString("%1._osc._tcp.local").arg(serviceName);
    qDebug() << QString("DNS Servicename: %1").arg(dnsServiceName);

    _dnsLookup = new QDnsLookup(this);

    _dnsLookup->setType(QDnsLookup::SRV);
    _dnsLookup->setName(dnsServiceName);

    connect(_dnsLookup, &QDnsLookup::finished, this, &Client::Watcher::onLookupFinished);

    _updateTimer = new QTimer(this);

    _updateTimer->setInterval(5000);

    connect(_updateTimer, &QTimer::timeout, this, &Client::Watcher::updateClients);

    // _updateTimer->start();

    updateClients();

    return 0;
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

void Watcher::onLookupFinished()
{
    // Process clients here
    qDebug() << "Processing lookup results";

    foreach (QDnsServiceRecord record, _dnsLookup->serviceRecords()) {
        qDebug().noquote() << QString("%1:%2 -> %3")
                                  .arg(record.name())
                                  .arg(record.port())
                                  .arg(record.target());

        ::Osc::Client* client = new ::Osc::Client(_manager, record.target(), record.port());
    }
}
}
