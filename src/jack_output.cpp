#include "jack_output.h"

#include <QDebug>

namespace Jack {
void Output::sendMidiNoteOn(quint8 channel, quint8 note, quint8 velocity) {}

void Output::sendMidiNoteOff(quint8 channel, quint8 note, quint8 velocity) {}

void Output::sendControlChange(quint8 channel, quint8 cc, quint8 value) {}

QString Output::portName()
{
    QString portName = jack_port_name(_port);

    return portName;
}

Output::Output(jack_port_t* port)
    : _port(port)
{
    
}

Output::~Output()
{
    qDebug() << "Closing jack port" << portName();
}
} // namespace Jack
