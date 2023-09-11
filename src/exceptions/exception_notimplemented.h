#pragma once

#include <string>
#include <string_view>

#include "exceptions/exception_genericaqualinkexception.h"

namespace AqualinkAutomate::Exceptions
{

	class NotImplemented : public GenericAqualinkException
	{
		static constexpr std::string_view NOT_IMPLEMENTED_MESSAGE{ "" };

	public:
		NotImplemented();
		NotImplemented(const std::string& message);
	};

}
// namespace AqualinkAutomate::Exceptions
