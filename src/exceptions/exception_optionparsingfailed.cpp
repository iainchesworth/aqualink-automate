#include <format>

#include "exceptions/exception_optionparsingfailed.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Exceptions
{

	OptionParsingFailed::OptionParsingFailed() :
		GenericAqualinkException(OPTION_PARSING_FAILED_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "OptionParsingFailed exception was constructed");
	}

	OptionParsingFailed::OptionParsingFailed(const std::string_view& message) :
		GenericAqualinkException(message)
	{
		LogTrace(Channel::Exceptions, std::format("OptionParsingFailed exception was constructed with message: {}", message));
	}

}
// namespace AqualinkAutomate::Exceptions
