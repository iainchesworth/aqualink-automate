#include <format>

#include "equipment/master_traffic_snoop.h"

namespace AqualinkAutomate::Equipment
{

	std::string FormatToMasterTraffic(const Messages::JandyMessage& message)
	{
		return std::format(
			"[to-master] cmd=0x{:02x} len={} chk=0x{:02x} | {}",
			message.RawId(),
			message.MessageLength(),
			message.ChecksumValue(),
			message.ToString());
	}

}
// namespace AqualinkAutomate::Equipment
