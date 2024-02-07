#pragma once

#include <string>

#include "exceptions/exception_genericaqualinkexception.h"

namespace AqualinkAutomate::Exceptions
{

	class Traits_InvalidTraitValue : public GenericAqualinkException
	{
		static constexpr std::string TRAIT_INVALID_TRAIT_VALUE_MESSAGE{ "" };

	public:
		Traits_InvalidTraitValue();
		Traits_InvalidTraitValue(const std::string& message);
	};

}
// namespace AqualinkAutomate::Exceptions
