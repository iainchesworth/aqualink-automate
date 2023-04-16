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
		auto output = std::format("Destination: 0x{:02x}", obj.DestinationId());

		os << output;
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyAckMessage& obj)
	{
		auto base_obj = dynamic_cast<const AqualinkAutomate::Messages::JandyMessage&>(obj);

		os << base_obj;
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyMessageMessage& obj)
	{
		auto base_obj = dynamic_cast<const AqualinkAutomate::Messages::JandyMessage&>(obj);

		os << base_obj;
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyMessageLongMessage& obj)
	{
		auto base_obj = dynamic_cast<const AqualinkAutomate::Messages::JandyMessage&>(obj);

		os << base_obj;
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyProbeMessage& obj)
	{
		auto base_obj = dynamic_cast<const AqualinkAutomate::Messages::JandyMessage&>(obj);

		os << base_obj;
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyStatusMessage& obj)
	{
		auto base_obj = dynamic_cast<const AqualinkAutomate::Messages::JandyMessage&>(obj);

		os << base_obj;
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::JandyUnknownMessage& obj)
	{
		auto base_obj = dynamic_cast<const AqualinkAutomate::Messages::JandyMessage&>(obj);

		os << base_obj;
		return os;
	}

}
// namespace std
