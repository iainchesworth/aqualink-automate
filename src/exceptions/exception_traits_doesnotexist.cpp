#include "exceptions/exception_traits_doesnotexist.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Exceptions
{

	Traits_DoesNotExist::Traits_DoesNotExist() :
		GenericAqualinkException(TRAIT_DOES_NOT_EXIST_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "Traits_DoesNotExist exception was constructed");
	}

	Traits_DoesNotExist::Traits_DoesNotExist(const std::string& message) :
		GenericAqualinkException(TRAIT_DOES_NOT_EXIST_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "Traits_DoesNotExist exception was constructed");
	}

}
// namespace AqualinkAutomate::Exceptions
