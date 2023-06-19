#pragma once

#include <string>
#include <string_view>

#include "exceptions/exception_optionparsingfailed.h"

namespace AqualinkAutomate::Exceptions
{

	class Options_MissingDependency : public OptionParsingFailed
	{
		static constexpr std::string_view OPTION_MISSING_DEPENDENCY_MESSAGE{ "" };

	public:
		Options_MissingDependency();
		Options_MissingDependency(const std::string& message);
	};

}
// namespace AqualinkAutomate::Exceptions
