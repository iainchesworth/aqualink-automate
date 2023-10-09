#include "exceptions/exception_http_duplicateroute.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Exceptions
{

    HTTP_DuplicateRoute::HTTP_DuplicateRoute() : GenericAqualinkException(DUPLICATE_ROUTE_MESSAGE)
    {
        LogTrace(Channel::Exceptions, "HTTP_DuplicateRoute exception was constructed");
    }

    HTTP_DuplicateRoute::HTTP_DuplicateRoute(const std::string &message) : GenericAqualinkException(DUPLICATE_ROUTE_MESSAGE)
    {
        LogTrace(Channel::Exceptions, "HTTP_DuplicateRoute exception was constructed");
    }

} 
// namespace AqualinkAutomate::Exceptions