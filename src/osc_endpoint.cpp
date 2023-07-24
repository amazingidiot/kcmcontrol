#include "osc_endpoint.h"

namespace Osc {
Endpoint::Endpoint(QRegularExpression pattern)
    : pattern { pattern }
{
}
}
