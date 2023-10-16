#include "exceptions/exception_certificate_notfound.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Exceptions
{

	Certificate_NotFound::Certificate_NotFound() :
		GenericAqualinkException(CERTIFICATE_NOT_FOUND_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "Certificate_NotFound exception was constructed");
	}

	Certificate_NotFound::Certificate_NotFound(const std::string& message) :
		GenericAqualinkException(CERTIFICATE_NOT_FOUND_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "Certificate_NotFound exception was constructed");
	}

}
// namespace AqualinkAutomate::Exceptions
