#pragma once

#include <QObject>
#include <jack/jack.h>
#include <jack/midiport.h>

namespace Jack {
class Client;

class Input : public QObject {
    Q_OBJECT

    friend class Jack::Client;

public:
    QString portName();

signals:
    void midiDataReceived();

private:
    explicit Input(jack_port_t* port, jack_client_t* client);
    ~Input();

    jack_port_t* _jack_port;
    jack_client_t* _jack_client;

    QQueue<QByteArray>* _queue;
signals:
};
} // namespace Jack
