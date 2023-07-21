#pragma once

#include <string>
#include <string_view>

#include "exceptions/exception_genericaqualinkexception.h"

namespace AqualinkAutomate::Exceptions
{

	class Traits_InvalidTraitValue : public GenericAqualinkException
	{
		static constexpr std::string_view TRAIT_INVALID_TRAIT_VALUE_MESSAGE{ "" };

	public:
		Traits_InvalidTraitValue();
		Traits_InvalidTraitValue(const std::string& message);
	};

}
// namespace AqualinkAutomate::Exceptions
