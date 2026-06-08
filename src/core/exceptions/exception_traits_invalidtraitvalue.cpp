#include "exceptions/exception_traits_invalidtraitvalue.h"
#include "logging/logging.h"

namespace AqualinkAutomate::Exceptions
{

	AQ_DEFINE_EXCEPTION(Traits_InvalidTraitValue, "The value requested from the trait does not match the trait's stored type.");

}
// namespace AqualinkAutomate::Exceptions
