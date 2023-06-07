#include "exceptions/exception_genericaqualinkexception.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Exceptions
{

	GenericAqualinkException::GenericAqualinkException(const std::string_view& message) :
		std::runtime_error(message.data())	
	{
		LogTrace(Channel::Exceptions, "GenericAqualinkException exception base was constructed");
	}

}
// namespace AqualinkAutomate::Exceptions
