#include "exceptions/exception_notimplemented.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Exceptions
{

	const std::string NotImplemented::NOT_IMPLEMENTED_MESSAGE{ "NOT_IMPLEMENTED_MESSAGE" };

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
