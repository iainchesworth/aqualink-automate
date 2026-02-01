#pragma once

#include <string>

#include "exceptions/exception_genericaqualinkexception.h"

namespace AqualinkAutomate::Exceptions
{

	class Traits_NotMutable : public GenericAqualinkException
	{
		static const std::string TRAIT_NOT_MUTABLE_MESSAGE;

	public:
		Traits_NotMutable();
		Traits_NotMutable(const std::string& message);
	};

}
// namespace AqualinkAutomate::Exceptions
