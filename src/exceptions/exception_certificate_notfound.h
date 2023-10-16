#pragma once

#include <string>
#include <string_view>

#include "exceptions/exception_genericaqualinkexception.h"

namespace AqualinkAutomate::Exceptions
{

	class Certificate_NotFound : public GenericAqualinkException
	{
		static constexpr std::string_view CERTIFICATE_NOT_FOUND_MESSAGE{ "" };

	public:
		Certificate_NotFound();
		Certificate_NotFound(const std::string& message);
	};

}
// namespace AqualinkAutomate::Exceptions
