#include "exceptions/exception_optionshelporversion.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Exceptions
{

	const std::string OptionsHelpOrVersion::OPTIONS_HELP_OR_VERSION_MESSAGE{ "OPTIONS_HELP_OR_VERSION_MESSAGE" };

	OptionsHelpOrVersion::OptionsHelpOrVersion() :
		GenericAqualinkException(OPTIONS_HELP_OR_VERSION_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "OptionsHelpOrVersion exception was constructed");
	}

}
// namespace AqualinkAutomate::Exceptions
