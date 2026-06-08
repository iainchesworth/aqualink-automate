#include "formatters/jandy_device_formatters.h"
#include "formatters/jandy_message_formatters.h"

namespace std
{
	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyMessageIds& obj)
	{
		os << std::format("{}", obj);
		return os;
	}

}
// namespace std
