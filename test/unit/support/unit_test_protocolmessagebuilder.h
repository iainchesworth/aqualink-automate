#pragma once

#include <cstdint>
#include <vector>

namespace AqualinkAutomate::Test
{

	class MessageBuilder
	{
	public:
		static std::vector<uint8_t> CreateValidMessage(uint8_t destination, uint8_t command, const std::vector<uint8_t>& payload)
		{
			std::vector<uint8_t> message;
			message.push_back(0x10); // Header
			message.push_back(0x02); // Header
			message.push_back(destination);
			message.push_back(command);

			for (auto byte : payload)
			{
				message.push_back(byte);
			}

			message.push_back(0x10); // Footer
			message.push_back(0x03); // Footer

			return message;
		}

		static std::vector<uint8_t> CreatePartialMessage(std::size_t bytes)
		{
			std::vector<uint8_t> message;
			if (bytes >= 1) message.push_back(0x10);
			if (bytes >= 2) message.push_back(0x02);
			if (bytes >= 3) message.push_back(0x42);
			if (bytes >= 4) message.push_back(0x03);

			for (std::size_t i = 4; i < bytes; ++i)
			{
				message.push_back(static_cast<uint8_t>(i & 0xFF));
			}

			return message;
		}

		static std::vector<uint8_t> CreateMultipleMessages(std::size_t count)
		{
			std::vector<uint8_t> data;

			for (std::size_t i = 0; i < count; ++i)
			{
				auto msg = CreateValidMessage(0x42, 0x03, { static_cast<uint8_t>(i) });
				data.insert(data.end(), msg.begin(), msg.end());
			}

			return data;
		}

		static std::vector<uint8_t> CreateGarbageData(std::size_t bytes)
		{
			std::vector<uint8_t> data(bytes);
			for (std::size_t i = 0; i < bytes; ++i)
			{
				data[i] = static_cast<uint8_t>((i * 17 + 42) & 0xFF);
			}
			return data;
		}

		static std::vector<uint8_t> CreateMessageWithChecksum(uint8_t destination, uint8_t command, const std::vector<uint8_t>& payload)
		{
			// For now, same as CreateValidMessage
			// Add checksum calculation if your protocol requires it
			return CreateValidMessage(destination, command, payload);
		}
	};

}
// namespace AqualinkAutomate::Test
