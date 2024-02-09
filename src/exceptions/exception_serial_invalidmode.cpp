#include "exceptions/exception_serial_invalidmode.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Exceptions
{

	const std::string Serial_InvalidMode::SERIAL_INVALID_MODE_MESSAGE{ "SERIAL_INVALID_MODE_MESSAGE" };

	Serial_InvalidMode::Serial_InvalidMode() :
		GenericAqualinkException(SERIAL_INVALID_MODE_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "Serial_InvalidMode exception was constructed");
	}

	Serial_InvalidMode::Serial_InvalidMode(const std::string& message) :
		GenericAqualinkException(SERIAL_INVALID_MODE_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "Serial_InvalidMode exception was constructed");
	}

}
// namespace AqualinkAutomate::Exceptions
