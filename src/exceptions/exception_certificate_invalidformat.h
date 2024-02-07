#pragma once

#include <string>

#include "exceptions/exception_genericaqualinkexception.h"

namespace AqualinkAutomate::Exceptions
{

	class Certificate_InvalidFormat : public GenericAqualinkException
	{
		static constexpr std::string CERTIFICATE_INVALID_FORMAT_MESSAGE{ "" };

	public:
		Certificate_InvalidFormat();
		Certificate_InvalidFormat(const std::string& message);
	};

}
// namespace AqualinkAutomate::Exceptions
