#include "osc_processor.h"
#include "client_watcher.h"
#include <QCoreApplication>

namespace Osc {
Processor::Processor(QObject* parent)
    : QObject { parent }
{
}

Processor::~Processor()
{
    qDebug() << "Destroying processor";

    for (int i = 0; i < _endpoints.length(); i++) {
        Osc::Endpoint* endpoint = _endpoints.at(i);
        endpoint->deleteLater();
    }

    _endpoints.clear();
}

int Processor::init(::Client::Watcher* watcher)
{
    _watcher = watcher;

    _jack_client = new Jack::Client(this);

    if (_jack_client->init("KCMControl") < 0) {
        qCritical() << "No jack client created";
        return -1;
    }
    _output_midi = _jack_client->addOutputPort("MIDI");
    _output_control = _jack_client->addOutputPort("Control");

#pragma region
    {
        Osc::Endpoint* endpoint = new Osc::Endpoint(QRegularExpression("^/noteon$"));
        QObject::connect(endpoint, &Osc::Endpoint::received, this, &Osc::Processor::endpointNoteOn);

        _endpoints.append(endpoint);
    }
    {
        Osc::Endpoint* endpoint = new Osc::Endpoint(QRegularExpression("^/noteoff$"));
        QObject::connect(endpoint, &Osc::Endpoint::received, this, &Osc::Processor::endpointNoteOff);

        _endpoints.append(endpoint);
    }
    {
        Osc::Endpoint* endpoint = new Osc::Endpoint(QRegularExpression("^/pedal/damper$"));
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
            &Osc::Processor::endpointPedalExpression);

        _endpoints.append(endpoint);
    }
    {
        Osc::Endpoint* endpoint = new Osc::Endpoint(QRegularExpression("^/watcher/update$"));
        QObject::connect(endpoint,
            &Osc::Endpoint::received,
            _watcher,
            &::Client::Watcher::updateClients);
        _endpoints.append(endpoint);
    }
    {
        Osc::Endpoint* endpoint = new Osc::Endpoint(QRegularExpression("^/quit$"));
        QObject::connect(endpoint,
            &Osc::Endpoint::received,
            QCoreApplication::instance(),
            &QCoreApplication::quit);

        _endpoints.append(endpoint);
    }
    {
        Osc::Endpoint* endpoint = new Osc::Endpoint(QRegularExpression(".*"));
        QObject::connect(endpoint,
            &Osc::Endpoint::received,
            this,
            &Osc::Processor::endpointDebugOutput);
        _endpoints.append(endpoint);
    }
#pragma endregion

    return 0;
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
    qDebug().noquote() << receivedMessage.toString();
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

    if (receivedMessage.format() != "iif") {
        qDebug() << "Invalid message for this endpoint:" << receivedMessage.toString();
        return;
    }

    int deviceIndex = receivedMessage.values.at(0).toInt();
    int note = receivedMessage.values.at(1).toInt();
    float velocity = receivedMessage.values.at(2).toFloat();

    _output_midi->sendMidiNoteOn(deviceIndex, note, (126 * velocity) + 1);

    // TODO: Implement Midi HighRes
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
