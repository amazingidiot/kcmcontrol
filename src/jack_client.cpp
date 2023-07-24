#include "jack_client.h"
#include "jack_output.h"
#include <QCoreApplication>
#include <QDebug>
namespace Jack {
Client::Client(QObject* parent)
    : QObject { parent }
{
}

Client::~Client()
{
    if (_jack_client == NULL) {
        qWarning() << "No jack client to close";

        return;
    }

    for (int i = 0; i < _input_ports.length(); i++) {
        Input* input = _input_ports.at(i);

        delete input;
    }

    for (int i = 0; i < _output_ports.length(); i++) {
        Output* output = _output_ports.at(i);

        delete output;
    }

    qDebug() << "Closing jack client";
    jack_client_close(_jack_client);
}

jack_client_t* Client::jack_client() const
{
    return _jack_client;
}

int Client::init(QString clientName)
{
    jack_options_t options;
    jack_status_t* status;

    qDebug() << "Creating jack client";
    if ((_jack_client = jack_client_open(clientName.toStdString().c_str(), JackNullOption, NULL))
        == 0) {
        qCritical() << "Failed to open jack client:" << status;
        return -1;
    }

    qDebug() << "Setting jack callbacks";
    jack_set_process_callback(_jack_client, Jack::Client::jack_callback, this);

    qDebug() << "Activating jack client";
    jack_activate(_jack_client);

    return 0;
}

Output* Client::addOutputPort(QString portName)
{
    if (_jack_client == NULL) {
        qWarning() << "No jack client available, can't create output port";

        return nullptr;
    }

    jack_port_t* port;

    qDebug() << "Creating midi port" << portName;
    if ((port = jack_port_register(_jack_client,
             portName.toStdString().c_str(),
             JACK_DEFAULT_MIDI_TYPE,
             JackPortFlags::JackPortIsOutput,
             0))
        == NULL) {
        qCritical() << "Could not register port" << portName;
        return nullptr;
    };

    Output* output = new Output(port, _jack_client);
    _output_ports.append(output);

    return output;
}

Input* Client::addInputPort(QString portName) { }

void Client::removeOutputPort(Output* output) { }

void Client::removeInputPort(Input* input) { }

int Client::jack_callback(jack_nframes_t nframes, void* arg)
{
    Jack::Client* client = static_cast<Jack::Client*>(arg);

    return 0;
}
} // namespace Jack
