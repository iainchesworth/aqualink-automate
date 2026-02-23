#include "exceptions/exception_webserver.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Exceptions
{

	const std::string WebServerException::OPTIONS_WEBSERVER_EXCEPTION_MESSAGE{ "OPTIONS_WEBSERVER_EXCEPTION_MESSAGE" };

	WebServerException::WebServerException(const std::string& message) :
		GenericAqualinkException(OPTIONS_WEBSERVER_EXCEPTION_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "WebServerException exception was constructed");
	}

}
// namespace AqualinkAutomate::Exceptions
