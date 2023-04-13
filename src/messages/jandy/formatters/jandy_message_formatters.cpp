#include "messages/jandy/formatters/jandy_message_formatters.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages::Jandy::Formatters
{

	// NOTHING HERE

}
// AqualinkAutomate::Messages::Jandy::Formatters

std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::Jandy::Messages::JandyMessage& obj)
{
	auto output = std::format("Destination: 0x{:02x}", obj.DestinationId());

	os << output;
	return os;
}

std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::Jandy::Messages::JandyAckMessage& obj)
{
	auto base_obj = dynamic_cast<const AqualinkAutomate::Messages::Jandy::Messages::JandyMessage&>(obj);

	os << base_obj;
	return os;
}

std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::Jandy::Messages::JandyMessageMessage& obj)
{
	auto base_obj = dynamic_cast<const AqualinkAutomate::Messages::Jandy::Messages::JandyMessage&>(obj);

	os << base_obj;
	return os;
}

std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::Jandy::Messages::JandyMessageLongMessage& obj)
{
	auto base_obj = dynamic_cast<const AqualinkAutomate::Messages::Jandy::Messages::JandyMessage&>(obj);

	os << base_obj;
	return os;
}

std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::Jandy::Messages::JandyProbeMessage& obj)
{
	auto base_obj = dynamic_cast<const AqualinkAutomate::Messages::Jandy::Messages::JandyMessage&>(obj);

	os << base_obj;
	return os;
}

std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::Jandy::Messages::JandyStatusMessage& obj)
{
	auto base_obj = dynamic_cast<const AqualinkAutomate::Messages::Jandy::Messages::JandyMessage&>(obj);

	os << base_obj;
	return os;
}

std::ostream& operator<<(std::ostream& os, const AqualinkAutomate::Messages::Jandy::Messages::JandyUnknownMessage& obj)
{
	auto base_obj = dynamic_cast<const AqualinkAutomate::Messages::Jandy::Messages::JandyMessage&>(obj);

	os << base_obj;
	return os;
}
