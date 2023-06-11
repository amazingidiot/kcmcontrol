#pragma once

#include <QObject>

#include <rtmidi/RtMidi.h>

namespace Midi {
class Output : public QObject {
    Q_OBJECT
public:
    explicit Output(QString clientName, QString portName, QObject *parent = nullptr);
    ~Output();

private:
    QString _portName = "";
    RtMidiOut *_midiout;
};
}
