#include "exceptions/exception_genericaqualinkexception.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Exceptions
{

	GenericAqualinkException::GenericAqualinkException(std::string message, std::source_location location, boost::stacktrace::stacktrace trace) :
		m_ExceptionMessage{ std::move(message) },
		m_SourceLocation{std::move(location) },
		m_StackTrace{ std::move(trace) }
	{
		LogTrace(Channel::Exceptions, "GenericAqualinkException exception base was constructed");
	}

	std::string& GenericAqualinkException::What()
	{
		return m_ExceptionMessage;
	}

	const std::string& GenericAqualinkException::What() const noexcept
	{
		return m_ExceptionMessage;
	}

	const std::source_location& GenericAqualinkException::Where() const noexcept
	{
		return m_SourceLocation;
	}

	const boost::stacktrace::stacktrace& GenericAqualinkException::StackTrace() const noexcept
	{
		return m_StackTrace;
	}

}
// namespace AqualinkAutomate::Exceptions
