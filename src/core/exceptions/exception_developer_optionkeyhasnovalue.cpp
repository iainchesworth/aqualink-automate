#include "exceptions/exception_developer_optionkeyhasnovalue.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Exceptions
{

	const std::string Developer_OptionKeyHasNoValue::DEVELOPER_OPTIONS_KEY_HAS_NO_VALUE_MESSAGE{ "DEVELOPER_OPTIONS_KEY_HAS_NO_VALUE_MESSAGE" };

	Developer_OptionKeyHasNoValue::Developer_OptionKeyHasNoValue() :
		GenericAqualinkException(DEVELOPER_OPTIONS_KEY_HAS_NO_VALUE_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "Developer_OptionKeyHasNoValue exception was constructed");
	}

	Developer_OptionKeyHasNoValue::Developer_OptionKeyHasNoValue(const std::string& message) :
		GenericAqualinkException(DEVELOPER_OPTIONS_KEY_HAS_NO_VALUE_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "Developer_OptionKeyHasNoValue exception was constructed");
	}

}
// namespace AqualinkAutomate::Exceptions
