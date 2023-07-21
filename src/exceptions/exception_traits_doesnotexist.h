#pragma once

#include <string>
#include <string_view>

#include "exceptions/exception_genericaqualinkexception.h"

namespace AqualinkAutomate::Exceptions
{

	class Traits_DoesNotExist : public GenericAqualinkException
	{
		static constexpr std::string_view TRAIT_DOES_NOT_EXIST_MESSAGE{ "" };

	public:
		Traits_DoesNotExist();
		Traits_DoesNotExist(const std::string& message);
	};

}
// namespace AqualinkAutomate::Exceptions
