#include "exceptions/exception_traits_invalidtraitvalue.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Exceptions
{

	const std::string Traits_InvalidTraitValue::TRAIT_INVALID_TRAIT_VALUE_MESSAGE{ "TRAIT_INVALID_TRAIT_VALUE_MESSAGE" };

	Traits_InvalidTraitValue::Traits_InvalidTraitValue() :
		GenericAqualinkException(TRAIT_INVALID_TRAIT_VALUE_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "Traits_InvalidTraitValue exception was constructed");
	}

	Traits_InvalidTraitValue::Traits_InvalidTraitValue(const std::string& message) :
		GenericAqualinkException(TRAIT_INVALID_TRAIT_VALUE_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "Traits_InvalidTraitValue exception was constructed");
	}

}
// namespace AqualinkAutomate::Exceptions
