#include "jandy/formatters/jandy_device_formatters.h"
#include "jandy/formatters/jandy_message_formatters.h"

namespace AqualinkAutomate::Formatters
{

	// NOTHING HERE

}
// AqualinkAutomate::Formatters

namespace std
{
	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyMessageIds& obj)
	{
		os << std::format("{}", obj);
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyMessage& obj)
	{
		os << std::format("{}", obj);
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyMessage_Ack& obj)
	{
		auto& base_obj = dynamic_cast<const AqualinkAutomate::Messages::JandyMessage&>(obj);

		os << base_obj;
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyMessage_Message& obj)
	{
		auto& base_obj = dynamic_cast<const AqualinkAutomate::Messages::JandyMessage&>(obj);

		os << base_obj;
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyMessage_MessageLong& obj)
	{
		auto& base_obj = dynamic_cast<const AqualinkAutomate::Messages::JandyMessage&>(obj);

		os << base_obj;
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyMessage_Probe& obj)
	{
		auto& base_obj = dynamic_cast<const AqualinkAutomate::Messages::JandyMessage&>(obj);

		os << base_obj;
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyMessage_Status& obj)
	{
		auto& base_obj = dynamic_cast<const AqualinkAutomate::Messages::JandyMessage&>(obj);

		os << base_obj;
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyMessage_Unknown& obj)
	{
		auto& base_obj = dynamic_cast<const AqualinkAutomate::Messages::JandyMessage&>(obj);

		os << base_obj;
		return os;
	}

}
// namespace std
