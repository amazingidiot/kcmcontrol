#include "osc_processor.h"

namespace Osc {
Processor::Processor(QObject *parent)
    : QObject{parent}
{
    _output_midi = new Midi::Output("KCMControl", "MIDI", this);
    _output_control = new Midi::Output("KCMControl", "Control", this);

#pragma region
/*
    Osc::Endpoint* endpoint = new Osc::Endpoint(
            QRegularExpression("^/noteon$")
                           );
        QObject::connect(endpoint, &Osc::Endpoint::received, this,
            &Osc::Server::endpoint_element_unsubscribe);
        _endpoints.append(endpoint);
    */
#pragma endregion
}

Processor::~Processor()
{
    qDebug() << "Destroying processor";
}

void Processor::process(Osc::Message message)
{
}
}
