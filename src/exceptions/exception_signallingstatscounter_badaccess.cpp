#include "exceptions/exception_signallingstatscounter_badaccess.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Exceptions
{

	const std::string SignallingStatsCounter_BadAccess::BAD_ACCESS_MESSAGE{ "BAD_ACCESS_MESSAGE" };

	SignallingStatsCounter_BadAccess::SignallingStatsCounter_BadAccess() :
		GenericAqualinkException(BAD_ACCESS_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "SignallingStatsCounter_BadAccess exception was constructed");
	}

	SignallingStatsCounter_BadAccess::SignallingStatsCounter_BadAccess(const std::string& message) :
		GenericAqualinkException(BAD_ACCESS_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "SignallingStatsCounter_BadAccess exception was constructed");
	}

}
// namespace AqualinkAutomate::Exceptions
