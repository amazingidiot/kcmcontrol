#pragma once

#include "osc_client.h"
#include <QDnsLookup>
#include <QDnsServiceRecord>
#include <QObject>
#include <QTcpSocket>
#include <QTimer>

namespace Client {
class Manager;

class Watcher : public QObject {
    Q_OBJECT
public:
    explicit Watcher(Manager* parent = nullptr);
    ~Watcher();

    int init(QString serviceName);

    quint32 updateInterval() const;
    void setUpdateInterval(quint32 newUpdateInterval);

signals:
    void clientConnected(Osc::Client* client);

public slots:
    void updateClients();

private:
    Manager* _manager;

    quint32 _updateInterval = 5000;

    QTimer* _updateTimer;

    QDnsLookup* _dnsLookup;
private slots:
    void onLookupFinished();
};
} // namespace Client
