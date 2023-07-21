#pragma once

#include <string>
#include <string_view>

#include "exceptions/exception_genericaqualinkexception.h"

namespace AqualinkAutomate::Exceptions
{

	class Traits_NotMutable : public GenericAqualinkException
	{
		static constexpr std::string_view TRAIT_NOT_MUTABLE_MESSAGE{ "" };

	public:
		Traits_NotMutable();
		Traits_NotMutable(const std::string& message);
	};

}
// namespace AqualinkAutomate::Exceptions
