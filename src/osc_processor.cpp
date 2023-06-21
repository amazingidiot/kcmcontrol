#include "osc_processor.h"

namespace Osc {
Processor::Processor(QObject *parent)
    : QObject{parent}
{
    _jack_client = new Jack::Client("KCMControl", this);

    _output_midi = _jack_client->addOutputPort("MIDI");
    _output_control = _jack_client->addOutputPort("Control");

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
