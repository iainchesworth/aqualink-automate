#pragma once

#include <source_location>
#include <string>

#include "exceptions/exception_genericaqualinkexception.h"

namespace AqualinkAutomate::Exceptions
{

	class WebServerException : public GenericAqualinkException
	{
	public:
		explicit WebServerException(const std::string& message, std::source_location location = std::source_location::current());
	};

}
// namespace AqualinkAutomate::Exceptions
