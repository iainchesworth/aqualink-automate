#pragma once

#include <string>

#include "exceptions/exception_optionparsingfailed.h"

namespace AqualinkAutomate::Exceptions
{

	class Options_ConflictingOptions : public OptionParsingFailed
	{
		static constexpr std::string OPTION_CONFLICTING_OPTIONS_MESSAGE{ "" };

	public:
		Options_ConflictingOptions();
		Options_ConflictingOptions(const std::string& message);
	};

}
// namespace AqualinkAutomate::Exceptions
