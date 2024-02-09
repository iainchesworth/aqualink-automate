#pragma once

#include <string>

#include "exceptions/exception_genericaqualinkexception.h"

namespace AqualinkAutomate::Exceptions
{

	class SignallingStatsCounter_BadAccess : public GenericAqualinkException
	{
		static const std::string BAD_ACCESS_MESSAGE;

	public:
		SignallingStatsCounter_BadAccess();
		SignallingStatsCounter_BadAccess(const std::string& message);
	};

}
// namespace AqualinkAutomate::Exceptions
