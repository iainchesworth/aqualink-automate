#include "exceptions/exception_traits_doesnotexist.h"
#include "logging/logging.h"

namespace AqualinkAutomate::Exceptions
{

	AQ_DEFINE_EXCEPTION(Traits_DoesNotExist, "The requested trait does not exist on this device.");

}
// namespace AqualinkAutomate::Exceptions
