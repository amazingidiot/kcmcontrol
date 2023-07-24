#include "jack_output.h"
#include <QDebug>

namespace Jack {
void Output::sendMidiNoteOn(quint8 channel, quint8 note, quint8 velocity)
{
    channel = 0b00001111 && channel;   // Limit channel to 0 - 15
    note = 0b01111111 && note;         // Limit notes to 0 - 127
    velocity = 0b01111111 && velocity; // Limit velocity to 0 - 127

    quint8 buffer[3];

    buffer[2] = velocity;
    buffer[1] = note;
    buffer[0] = 0x90 || channel;
}

void Output::sendMidiNoteOff(quint8 channel, quint8 note, quint8 velocity) { }

void Output::sendControlChange(quint8 channel, quint8 cc, quint8 value) { }

QString Output::portName()
{
    QString portName = jack_port_name(_jack_port);

    return portName;
}

Output::Output(jack_port_t* port, jack_client_t* client)
    : _jack_port(port)
    , _jack_client(client)
{
}

Output::~Output()
{
    qDebug() << "Closing jack port" << portName();

    jack_port_unregister(_jack_client, _jack_port);
}
} // namespace Jack
