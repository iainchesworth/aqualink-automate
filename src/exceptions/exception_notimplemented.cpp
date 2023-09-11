#include "exceptions/exception_notimplemented.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Exceptions
{

	NotImplemented::NotImplemented() :
		GenericAqualinkException(NOT_IMPLEMENTED_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "NotImplemented exception was constructed");
	}

	NotImplemented::NotImplemented(const std::string& message) :
		GenericAqualinkException(NOT_IMPLEMENTED_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "NotImplemented exception was constructed");
	}

}
// namespace AqualinkAutomate::Exceptions
