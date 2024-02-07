#pragma once

#include <string>

#include "exceptions/exception_genericaqualinkexception.h"

namespace AqualinkAutomate::Exceptions
{

	class OptionParsingFailed : public GenericAqualinkException
	{
		static constexpr std::string OPTION_PARSING_FAILED_MESSAGE{ "" };

	public:
		OptionParsingFailed();
		OptionParsingFailed(const std::string& message);
	};

}
// namespace AqualinkAutomate::Exceptions
