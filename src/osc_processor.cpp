#include "osc_processor.h"

namespace Osc {
Processor::Processor(QObject *parent)
    : QObject{parent}
{
    _jack_client = new Jack::Client("KCMControl", this);

    _output_midi = _jack_client->addOutputPort("MIDI");
    _output_control = _jack_client->addOutputPort("Control");

#pragma region
    {
        Osc::Endpoint* endpoint = new Osc::Endpoint(QRegularExpression("^/noteon$"));
        QObject::connect(endpoint,
                         &Osc::Endpoint::received,
                         this,
                         &Osc::Processor::endpointDebugOutput);
        QObject::connect(endpoint, &Osc::Endpoint::received, this, &Osc::Processor::endpointNoteOn);

        _endpoints.append(endpoint);
    }
    {
        Osc::Endpoint* endpoint = new Osc::Endpoint(QRegularExpression("^/noteoff$"));
        QObject::connect(endpoint,
                         &Osc::Endpoint::received,
                         this,
                         &Osc::Processor::endpointDebugOutput);
        QObject::connect(endpoint, &Osc::Endpoint::received, this, &Osc::Processor::endpointNoteOff);

        _endpoints.append(endpoint);
    }
    {
        Osc::Endpoint* endpoint = new Osc::Endpoint(QRegularExpression("^/pedal/damper$"));
        QObject::connect(endpoint,
                         &Osc::Endpoint::received,
                         this,
                         &Osc::Processor::endpointDebugOutput);
        QObject::connect(endpoint,
                         &Osc::Endpoint::received,
                         this,
                         &Osc::Processor::endpointPedalDamper);

        _endpoints.append(endpoint);
    }
    {
        Osc::Endpoint* endpoint = new Osc::Endpoint(QRegularExpression("^/pedal/expression$"));
        QObject::connect(endpoint,
                         &Osc::Endpoint::received,
                         this,
                         &Osc::Processor::endpointDebugOutput);
        QObject::connect(endpoint,
                         &Osc::Endpoint::received,
                         this,
                         &Osc::Processor::endpointPedalExpression);

        _endpoints.append(endpoint);
    }
#pragma endregion
}

Processor::~Processor()
{
    qDebug() << "Destroying processor";

    for (int i = 0; i < _endpoints.length(); i++) {
        Osc::Endpoint* endpoint = _endpoints.at(i);

        delete endpoint;
    }
}

void Processor::process(Osc::Message message)
{
    foreach (Osc::Endpoint* endpoint, _endpoints) {
        if (endpoint->pattern.match(message.address).hasMatch()) {
            emit endpoint->received(message);
        }
    }
}

void Processor::endpointDebugOutput(Message receivedMessage)
{
    qDebug() << receivedMessage.toString();
}

void Processor::endpointNoteOn(Message receivedMessage)
{
    /*
     * Values:
     * 
     * i    Device Index
     * i    Note
     * f    Velocity
     * 
     */
}

void Processor::endpointNoteOff(Message receivedMessage)
{
    /*
     * Values:onReceiveData
     * 
     * i    Device Index
     * i    Note
     * f    Velocity
     * 
     */
}

void Processor::endpointPedalDamper(Message receivedMessage)
{
    /*
     * Values:
     * 
     * i    Device Index
     * b    Pressed
     * 
     */
}

void Processor::endpointPedalExpression(Message receivedMessage)
{
    /*
     * Values:
     * 
     * i    Device Index
     * f    Expression value
     * 
     */
}
}
