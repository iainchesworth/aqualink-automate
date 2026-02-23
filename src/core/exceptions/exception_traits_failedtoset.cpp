#include "exceptions/exception_traits_failedtoset.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Exceptions
{

	const std::string Traits_FailedToSet::TRAIT_FAILED_TO_SET_MESSAGE{ "TRAIT_FAILED_TO_SET_MESSAGE" };

	Traits_FailedToSet::Traits_FailedToSet() :
		GenericAqualinkException(TRAIT_FAILED_TO_SET_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "Traits_FailedToSet exception was constructed");
	}

	Traits_FailedToSet::Traits_FailedToSet(const std::string& message) :
		GenericAqualinkException(TRAIT_FAILED_TO_SET_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "Traits_FailedToSet exception was constructed");
	}

}
// namespace AqualinkAutomate::Exceptions
