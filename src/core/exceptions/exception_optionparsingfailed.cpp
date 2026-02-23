#include <format>

#include "exceptions/exception_optionparsingfailed.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Exceptions
{

	const std::string OptionParsingFailed::OPTION_PARSING_FAILED_MESSAGE{ "OPTION_PARSING_FAILED_MESSAGE" };

	OptionParsingFailed::OptionParsingFailed() :
		GenericAqualinkException(OPTION_PARSING_FAILED_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "OptionParsingFailed exception was constructed");
	}

	OptionParsingFailed::OptionParsingFailed(const std::string& message) :
		GenericAqualinkException(message)
	{
		LogTrace(Channel::Exceptions, std::format("OptionParsingFailed exception was constructed with message: {}", message));
	}

}
// namespace AqualinkAutomate::Exceptions
