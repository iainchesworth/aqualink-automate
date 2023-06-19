#pragma once

#include <string_view>

#include "exceptions/exception_genericaqualinkexception.h"

namespace AqualinkAutomate::Exceptions
{

	class OptionParsingFailed : public GenericAqualinkException
	{
		static constexpr std::string_view OPTION_PARSING_FAILED_MESSAGE{ "" };

	public:
		OptionParsingFailed();

	protected:
		OptionParsingFailed(const std::string_view& message);
	};

}
// namespace AqualinkAutomate::Exceptions
