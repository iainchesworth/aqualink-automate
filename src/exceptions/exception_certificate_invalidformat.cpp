#include "exceptions/exception_certificate_invalidformat.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Exceptions
{

	const std::string Certificate_InvalidFormat::CERTIFICATE_INVALID_FORMAT_MESSAGE{ "CERTIFICATE_INVALID_FORMAT_MESSAGE" };

	Certificate_InvalidFormat::Certificate_InvalidFormat() :
		GenericAqualinkException(CERTIFICATE_INVALID_FORMAT_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "Certificate_InvalidFormat exception was constructed");
	}

	Certificate_InvalidFormat::Certificate_InvalidFormat(const std::string& message) :
		GenericAqualinkException(CERTIFICATE_INVALID_FORMAT_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "Certificate_InvalidFormat exception was constructed");
	}

}
// namespace AqualinkAutomate::Exceptions
