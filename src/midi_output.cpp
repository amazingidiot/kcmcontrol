#include "midi_output.h"

#include <QDebug>

namespace Midi {
Output::Output(QString clientName, QString portName, QObject *parent)
    : QObject{parent}
    , _portName(portName)
{
    qDebug() << "Creating midi port" << _portName << "as" << clientName;

    _midiout = new RtMidiOut(RtMidi::Api::UNIX_JACK, clientName.toStdString());

    _midiout->openVirtualPort(_portName.toStdString());
}

Output::~Output()
{
    qDebug() << "Destroying midi output port" << _portName;

    _midiout->closePort();
}
}
