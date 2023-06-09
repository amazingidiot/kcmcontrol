#include "osc_client.h"
#include <memory>
#include <qobject.h>

Osc::Client::Client(QHostAddress address, quint16 port) {
  this->_address = address;
  this->_port = port;
  this->_heartbeat = QTime::currentTime();
}
Osc::Client::~Client() {}

QHostAddress Osc::Client::address() { return _address; }

quint16 Osc::Client::port() { return _port; }

QTime Osc::Client::heartbeat() { return _heartbeat; }

void Osc::Client::setHeartbeat() { this->_heartbeat = QTime::currentTime(); }
