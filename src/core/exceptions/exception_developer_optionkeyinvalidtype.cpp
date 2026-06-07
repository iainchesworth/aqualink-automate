#include "exceptions/exception_developer_optionkeyinvalidtype.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Exceptions
{

	const std::string Developer_OptionKeyInvalidType::DEVELOPER_OPTIONS_KEY_INVALID_TYPE_MESSAGE{ "DEVELOPER_OPTIONS_KEY_INVALID_TYPE_MESSAGE" };

	Developer_OptionKeyInvalidType::Developer_OptionKeyInvalidType() :
		GenericAqualinkException(DEVELOPER_OPTIONS_KEY_INVALID_TYPE_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "Developer_OptionKeyInvalidType exception was constructed");
	}

	Developer_OptionKeyInvalidType::Developer_OptionKeyInvalidType(const std::string& message) :
		GenericAqualinkException(message)
	{
		LogTrace(Channel::Exceptions, "Developer_OptionKeyInvalidType exception was constructed");
	}

}
// namespace AqualinkAutomate::Exceptions
