#include "exceptions/exception_genericaqualinkexception.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Exceptions
{

	GenericAqualinkException::GenericAqualinkException(const std::string_view& message) :
		std::runtime_error{ message.data() },
		m_StackTrace{ std::stacktrace::current(1 /* skipped frames */) }	
	{
		LogTrace(Channel::Exceptions, "GenericAqualinkException exception base was constructed");
	}

	const std::stacktrace& GenericAqualinkException::StackTrace() const
	{
		return m_StackTrace;
	}

}
// namespace AqualinkAutomate::Exceptions
