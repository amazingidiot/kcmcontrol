#pragma once

#include "jack_client.h"
#include "jack_output.h"
#include "osc_endpoint.h"
#include "osc_message.h"

#include <QObject>

namespace Osc {
class Processor : public QObject {
    Q_OBJECT
public:
    explicit Processor(QObject* parent = nullptr);
    ~Processor();

public slots:
    void process(Osc::Message message);

signals:

private:
    QList<Osc::Endpoint*> _endpoints;

    Jack::Client* _jack_client;
    Jack::Output* _output_midi;
    Jack::Output* _output_control;
};
}
