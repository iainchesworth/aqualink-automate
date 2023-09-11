#pragma once

#include <string>
#include <string_view>

#include "exceptions/exception_genericaqualinkexception.h"

namespace AqualinkAutomate::Exceptions
{

	class Serial_InvalidMode : public GenericAqualinkException
	{
		static constexpr std::string_view SERIAL_INVALID_MODE_MESSAGE{ "" };

	public:
		Serial_InvalidMode();
		Serial_InvalidMode(const std::string& message);
	};

}
// namespace AqualinkAutomate::Exceptions
