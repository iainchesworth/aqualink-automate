#include "exceptions/exception_options_missingdependency.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Exceptions
{

	const std::string Options_MissingDependency::OPTION_MISSING_DEPENDENCY_MESSAGE{ "OPTION_MISSING_DEPENDENCY_MESSAGE" };

	Options_MissingDependency::Options_MissingDependency() :
		OptionParsingFailed(OPTION_MISSING_DEPENDENCY_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "Options_MissingDependency exception was constructed");
	}

	Options_MissingDependency::Options_MissingDependency(const std::string& message) :
		OptionParsingFailed(OPTION_MISSING_DEPENDENCY_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "Options_MissingDependency exception was constructed");
	}

}
// namespace AqualinkAutomate::Exceptions
