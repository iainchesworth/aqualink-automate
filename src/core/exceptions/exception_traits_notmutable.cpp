#include "exceptions/exception_traits_notmutable.h"
#include "logging/logging.h"

namespace AqualinkAutomate::Exceptions
{

	AQ_DEFINE_EXCEPTION(Traits_NotMutable, "This trait is read-only and cannot be modified.");

}
// namespace AqualinkAutomate::Exceptions
