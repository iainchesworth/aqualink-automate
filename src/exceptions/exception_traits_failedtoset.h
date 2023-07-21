#pragma once

#include <string>
#include <string_view>

#include "exceptions/exception_genericaqualinkexception.h"

namespace AqualinkAutomate::Exceptions
{

	class Traits_FailedToSet : public GenericAqualinkException
	{
		static constexpr std::string_view TRAIT_FAILED_TO_SET_MESSAGE{ "" };

	public:
		Traits_FailedToSet();
		Traits_FailedToSet(const std::string& message);
	};

}
// namespace AqualinkAutomate::Exceptions
