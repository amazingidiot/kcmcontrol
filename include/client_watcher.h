#pragma once

#include "osc_client.h"

#include <QDnsLookup>
#include <QDnsServiceRecord>
#include <QObject>
#include <QTcpSocket>
#include <QTimer>

#include <memory>
namespace Client {
class Watcher : public QObject
{
    Q_OBJECT
public:
    explicit Watcher(QString serviceName, QObject *parent = nullptr);
    ~Watcher();

    quint32 updateInterval() const;
    void setUpdateInterval(quint32 newUpdateInterval);

signals:
    void clientConnected(std::shared_ptr<Osc::Client> client);

private:
    quint32 _updateInterval = 5000;

    QTimer *_updateTimer;

    QDnsLookup *_dnsLookup;
private slots:
    void updateClients();
    void lookupFinished();
};
} // namespace Client
