#pragma once

#include <string>

#include "exceptions/exception_genericaqualinkexception.h"

namespace AqualinkAutomate::Exceptions
{

	class Serial_InvalidMode : public GenericAqualinkException
	{
		static const std::string SERIAL_INVALID_MODE_MESSAGE;

	public:
		Serial_InvalidMode();
		Serial_InvalidMode(const std::string& message);
	};

}
// namespace AqualinkAutomate::Exceptions
