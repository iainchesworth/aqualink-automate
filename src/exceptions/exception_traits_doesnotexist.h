#pragma once

#include <string>

#include "exceptions/exception_genericaqualinkexception.h"

namespace AqualinkAutomate::Exceptions
{

	class Traits_DoesNotExist : public GenericAqualinkException
	{
		static constexpr std::string TRAIT_DOES_NOT_EXIST_MESSAGE{ "" };

	public:
		Traits_DoesNotExist();
		Traits_DoesNotExist(const std::string& message);
	};

}
// namespace AqualinkAutomate::Exceptions
