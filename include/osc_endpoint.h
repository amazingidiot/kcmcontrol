#pragma once

#include "osc_message.h"
#include <QObject>
#include <QRegularExpression>

namespace Osc {
class Endpoint : public QObject {
    Q_OBJECT

public:
    Endpoint(QRegularExpression pattern);
    QRegularExpression pattern;

signals:
    bool received(Osc::Message received_message);
};
}
