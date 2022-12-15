#pragma once

#include <stdexcept>
#include <string_view>

namespace AqualinkAutomate::Exceptions
{

	class GenericAqualinkException : public std::runtime_error
	{
	public:
		GenericAqualinkException(const std::string_view& message);
	};

}
// namespace AqualinkAutomate::Exceptions
