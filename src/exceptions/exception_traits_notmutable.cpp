#include "exceptions/exception_traits_notmutable.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Exceptions
{

	const std::string Traits_NotMutable::TRAIT_NOT_MUTABLE_MESSAGE{ "TRAIT_NOT_MUTABLE_MESSAGE" };

	Traits_NotMutable::Traits_NotMutable() :
		GenericAqualinkException(TRAIT_NOT_MUTABLE_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "Traits_NotMutable exception was constructed");
	}

	Traits_NotMutable::Traits_NotMutable(const std::string& message) :
		GenericAqualinkException(TRAIT_NOT_MUTABLE_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "Traits_NotMutable exception was constructed");
	}

}
// namespace AqualinkAutomate::Exceptions
