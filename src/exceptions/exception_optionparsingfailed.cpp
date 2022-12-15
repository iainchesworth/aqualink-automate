#include "exceptions/exception_optionparsingfailed.h"
#include "logging/logging.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Exceptions
{

	OptionParsingFailed::OptionParsingFailed() :
		GenericAqualinkException(OPTION_PARSING_FAILED_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "OptionParsingFailed exception was constructed");
	}

}
// namespace AqualinkAutomate::Exceptions
