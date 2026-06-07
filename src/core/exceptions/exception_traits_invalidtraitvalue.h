#pragma once

#include <string>

#include "exceptions/exception_genericaqualinkexception.h"

namespace AqualinkAutomate::Exceptions
{

	class Traits_InvalidTraitValue : public GenericAqualinkException
	{
		static const std::string TRAIT_INVALID_TRAIT_VALUE_MESSAGE;

	public:
		Traits_InvalidTraitValue();
	};

}
// namespace AqualinkAutomate::Exceptions
