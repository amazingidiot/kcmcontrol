#pragma once

#include <QObject>

#include <jack/jack.h>
#include <jack/midiport.h>

namespace Jack {
class Client;

class Output : public QObject {
    Q_OBJECT

    friend class Jack::Client;

public:
    void sendMidiNoteOn(quint8 channel, quint8 note, quint8 velocity);
    void sendMidiNoteOff(quint8 channel, quint8 note, quint8 velocity);
    void sendControlChange(quint8 channel, quint8 cc, quint8 value);

    QString portName();

private:
    explicit Output(jack_port_t* port);
    ~Output();

    jack_port_t* _port;
};
} // namespace Jack
