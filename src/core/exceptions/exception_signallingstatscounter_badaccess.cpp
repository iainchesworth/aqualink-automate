#include "exceptions/exception_signallingstatscounter_badaccess.h"
#include "logging/logging.h"

namespace AqualinkAutomate::Exceptions
{

	AQ_DEFINE_EXCEPTION(SignallingStatsCounter_BadAccess, "The requested statistics counter entry does not exist.");

}
// namespace AqualinkAutomate::Exceptions
