#include "exceptions/exception_hubnotfound.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Exceptions
{

	Hub_NotFound::Hub_NotFound() :
		GenericAqualinkException(HUB_NOT_FOUND_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "Hub_NotFound exception was constructed");
	}

	Hub_NotFound::Hub_NotFound(const std::string& message) :
		GenericAqualinkException(HUB_NOT_FOUND_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "Hub_NotFound exception was constructed");
	}

}
// namespace AqualinkAutomate::Exceptions