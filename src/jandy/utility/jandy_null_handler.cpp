#include "jandy/messages/jandy_message.h"
#include "jandy/messages/jandy_message_constants.h"
#include "jandy/utility/jandy_null_handler.h"

namespace AqualinkAutomate::Utility
{

	void JandyPacket_NullCharHandler_Serialization(std::vector<uint8_t>& message_bytes)
	{
		if (Messages::JandyMessage::MINIMUM_PACKET_LENGTH > message_bytes.size())
		{
			// Packet is too small (technically an error but don't handle it here).
		}
		else 
		{
			auto start_it = std::next(message_bytes.begin(), 2);
			auto end_it = std::next(message_bytes.rbegin(), 3).base();

			for (auto it = start_it; it != end_it; ++it) 
			{
				if (Messages::HEADER_BYTE_DLE  == *it)
				{
					it = message_bytes.insert(std::next(it), 0x00);
					end_it = std::next(message_bytes.rbegin(), 3).base();
				}
			}
		}
	}

	void JandyPacket_NullCharHandler_Deserialization(std::vector<uint8_t>& message_bytes)
	{
		bool last_byte_was_0x10 = false;

		std::erase_if
		(
			message_bytes,
			[&last_byte_was_0x10](uint8_t current_byte) -> bool
			{
				bool should_remove = (last_byte_was_0x10 && (0x00 == current_byte));
				last_byte_was_0x10 = (0x10 == current_byte);
				return should_remove;
			}
		);
	}

}
// namespace AqualinkAutomate::Utility
