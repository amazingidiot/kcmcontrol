#pragma once

#include "jack_input.h"
#include "jack_output.h"
#include <QObject>
#include <jack/jack.h>

namespace Jack {
class Output;

class Client : public QObject {
    Q_OBJECT
public:
    explicit Client(QObject* parent = nullptr);
    ~Client();

    jack_client_t* jack_client() const;

    int init(QString clientName);

public slots:
    Jack::Output* addOutputPort(QString portName);
    Jack::Input* addInputPort(QString portName);

    void removeOutputPort(Jack::Output* output);
    void removeInputPort(Jack::Input* input);
signals:

private:
    static int jack_callback(jack_nframes_t nframes, void* arg);

    jack_client_t* _jack_client;

    QList<Input*> _input_ports;
    QList<Output*> _output_ports;
};
} // namespace Jack
