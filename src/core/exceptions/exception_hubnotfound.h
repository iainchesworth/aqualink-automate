#pragma once

#include <string>

#include "exceptions/exception_genericaqualinkexception.h"

namespace AqualinkAutomate::Exceptions
{

	class Hub_NotFound : public GenericAqualinkException
	{
		static const std::string HUB_NOT_FOUND_MESSAGE;

	public:
		Hub_NotFound();
		Hub_NotFound(const std::string& message);
	};

}
// namespace AqualinkAutomate::Exceptions
