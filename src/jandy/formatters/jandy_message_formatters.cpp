#include <magic_enum.hpp>

#include "jandy/formatters/jandy_message_formatters.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Formatters
{

	// NOTHING HERE

}
// AqualinkAutomate::Formatters

namespace std
{

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyMessage& obj)
	{
		auto output = std::format
		(
			"Destination: {} (0x{:02x}), Message Type: {} (0x{:02x})", 
			magic_enum::enum_name(obj.Destination().Class()), 
			obj.Destination().Raw(), 
			magic_enum::enum_name(obj.Id()),
			obj.RawId()
		);

		os << output;
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyMessage_Ack& obj)
	{
		auto base_obj = dynamic_cast<const AqualinkAutomate::Messages::JandyMessage&>(obj);

		os << base_obj;
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyMessage_Message& obj)
	{
		auto base_obj = dynamic_cast<const AqualinkAutomate::Messages::JandyMessage&>(obj);

		os << base_obj;
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyMessage_MessageLong& obj)
	{
		auto base_obj = dynamic_cast<const AqualinkAutomate::Messages::JandyMessage&>(obj);

		os << base_obj;
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyMessage_Probe& obj)
	{
		auto base_obj = dynamic_cast<const AqualinkAutomate::Messages::JandyMessage&>(obj);

		os << base_obj;
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyMessage_Status& obj)
	{
		auto base_obj = dynamic_cast<const AqualinkAutomate::Messages::JandyMessage&>(obj);

		os << base_obj;
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyMessage_Unknown& obj)
	{
		auto base_obj = dynamic_cast<const AqualinkAutomate::Messages::JandyMessage&>(obj);

		os << base_obj;
		return os;
	}

}
// namespace std
