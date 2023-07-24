#include "jack_input.h"
#include <QDebug>

namespace Jack {
Input::Input(jack_port_t* port, jack_client_t* client)
    : _jack_port(port)
    , _jack_client(client)
{
}

Input::~Input()
{
    qDebug() << "Closing jack input port" << portName();

    jack_port_unregister(_jack_client, _jack_port);
}

QString Input::portName()
{
    QString portName = jack_port_name(_jack_port);

    return portName;
}
} // namespace Jack
