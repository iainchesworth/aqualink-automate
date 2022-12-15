#include "exceptions/exception_optionshelporversion.h"
#include "logging/logging.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Exceptions
{

	OptionsHelpOrVersion::OptionsHelpOrVersion() :
		GenericAqualinkException(OPTIONS_HELP_OR_VERSION_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "OptionsHelpOrVersion exception was constructed");
	}

}
// namespace AqualinkAutomate::Exceptions
