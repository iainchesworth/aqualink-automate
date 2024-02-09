#pragma once

#include <string>

#include "exceptions/exception_genericaqualinkexception.h"

namespace AqualinkAutomate::Exceptions
{

	class Certificate_NotFound : public GenericAqualinkException
	{
		static const std::string CERTIFICATE_NOT_FOUND_MESSAGE;

	public:
		Certificate_NotFound();
		Certificate_NotFound(const std::string& message);
	};

}
// namespace AqualinkAutomate::Exceptions
