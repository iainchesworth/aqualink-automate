#pragma once

#include <string_view>

#include "exceptions/exception_genericaqualinkexception.h"

namespace AqualinkAutomate::Exceptions
{

	class WebServerException : public GenericAqualinkException
	{
		static constexpr std::string_view OPTIONS_WEBSERVER_EXCEPTION_MESSAGE{ "" };

	public:
		WebServerException(const std::string& message);
	};

}
// namespace AqualinkAutomate::Exceptions
