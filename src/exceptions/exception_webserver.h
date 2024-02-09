#pragma once

#include <string>

#include "exceptions/exception_genericaqualinkexception.h"

namespace AqualinkAutomate::Exceptions
{

	class WebServerException : public GenericAqualinkException
	{
		static const std::string OPTIONS_WEBSERVER_EXCEPTION_MESSAGE;

	public:
		WebServerException(const std::string& message);
	};

}
// namespace AqualinkAutomate::Exceptions
