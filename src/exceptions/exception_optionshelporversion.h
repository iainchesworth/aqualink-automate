#pragma once

#include <string>

#include "exceptions/exception_genericaqualinkexception.h"

namespace AqualinkAutomate::Exceptions
{

	class OptionsHelpOrVersion : public GenericAqualinkException
	{
		static constexpr std::string OPTIONS_HELP_OR_VERSION_MESSAGE{ "" };

	public:
		OptionsHelpOrVersion();
		OptionsHelpOrVersion(const std::string& message);
	};

}
// namespace AqualinkAutomate::Exceptions
