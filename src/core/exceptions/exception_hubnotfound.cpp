#include "exceptions/exception_hubnotfound.h"
#include "logging/logging.h"

namespace AqualinkAutomate::Exceptions
{

	AQ_DEFINE_EXCEPTION(Hub_NotFound, "The requested hub is not registered with the hub locator.");

}
// namespace AqualinkAutomate::Exceptions
