#pragma once

#include <string_view>

#include "exceptions/exception_genericaqualinkexception.h"

namespace AqualinkAutomate::Exceptions
{

	class OptionsHelpOrVersion : public GenericAqualinkException
	{
		static constexpr std::string_view OPTIONS_HELP_OR_VERSION_MESSAGE{ "" };

	public:
		OptionsHelpOrVersion();
	};

}
// namespace AqualinkAutomate::Exceptions
