#include "exceptions/exception_webserver.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Exceptions
{

	WebServerException::WebServerException(const std::string& message) :
		GenericAqualinkException(OPTIONS_WEBSERVER_EXCEPTION_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "WebServerException exception was constructed");
	}

}
// namespace AqualinkAutomate::Exceptions
