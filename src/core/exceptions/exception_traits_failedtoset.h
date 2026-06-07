#pragma once

#include <string>

#include "exceptions/exception_genericaqualinkexception.h"

namespace AqualinkAutomate::Exceptions
{

	class Traits_FailedToSet : public GenericAqualinkException
	{
		static const std::string TRAIT_FAILED_TO_SET_MESSAGE;

	public:
		Traits_FailedToSet();
	};

}
// namespace AqualinkAutomate::Exceptions
