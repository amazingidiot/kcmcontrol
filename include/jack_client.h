#pragma once

#include "jack_output.h"

#include <QObject>

#include <jack/jack.h>

namespace Jack {
class Output;

class Client : public QObject
{
    Q_OBJECT
public:
    explicit Client(QString clientName = "KCMControl", QObject* parent = nullptr);
    ~Client();

    jack_client_t* jack_client() const;

public slots:
    Jack::Output* addOutputPort(QString portName);
    void removeOutputPort(Jack::Output* output);

signals:

private:
    static int jack_callback(jack_nframes_t nframes, void* arg);

    jack_client_t* _jack_client;

    QList<Output*> _output_ports;
};
} // namespace Jack
