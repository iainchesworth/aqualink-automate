#include "exceptions/exception_options_conflictingoptions.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Exceptions
{

	const std::string Options_ConflictingOptions::OPTION_CONFLICTING_OPTIONS_MESSAGE{ "OPTION_CONFLICTING_OPTIONS_MESSAGE" };

	Options_ConflictingOptions::Options_ConflictingOptions() :
		OptionParsingFailed(OPTION_CONFLICTING_OPTIONS_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "Options_ConflictingOptions exception was constructed");
	}

	Options_ConflictingOptions::Options_ConflictingOptions(const std::string& message) :
		OptionParsingFailed(OPTION_CONFLICTING_OPTIONS_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "Options_ConflictingOptions exception was constructed");
	}

}
// namespace AqualinkAutomate::Exceptions
