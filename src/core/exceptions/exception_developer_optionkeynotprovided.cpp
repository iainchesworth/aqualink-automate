#include "exceptions/exception_developer_optionkeynotprovided.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Exceptions
{

	const std::string Developer_OptionKeyNotProvided::DEVELOPER_OPTIONS_KEY_NOT_PROVIDED_MESSAGE{ "DEVELOPER_OPTIONS_KEY_NOT_PROVIDED_MESSAGE" };

	Developer_OptionKeyNotProvided::Developer_OptionKeyNotProvided() :
		GenericAqualinkException(DEVELOPER_OPTIONS_KEY_NOT_PROVIDED_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "Developer_OptionKeyNotProvided exception was constructed");
	}

	Developer_OptionKeyNotProvided::Developer_OptionKeyNotProvided(const std::string& message) :
		GenericAqualinkException(DEVELOPER_OPTIONS_KEY_NOT_PROVIDED_MESSAGE)
	{
		LogTrace(Channel::Exceptions, "Developer_OptionKeyNotProvided exception was constructed");
	}

}
// namespace AqualinkAutomate::Exceptions
