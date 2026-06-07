#pragma once

#include <cstdint>
#include <vector>

#include "jandy/utility/jandy_checksum.h"
#include "pentair/utility/pentair_checksum.h"

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

		// Build a fully-framed Jandy RS-485 packet WITH a valid checksum byte.
		//
		// Wire layout: [DLE=0x10][STX=0x02][dest][cmd][payload...][checksum][DLE=0x10][ETX=0x03]
		//
		// The checksum is the low byte of the sum over every byte from the leading
		// DLE up to (and including) the last payload byte — i.e. everything that
		// precedes the checksum byte itself.  This mirrors exactly what the Jandy
		// message generator validates in PacketValidation_ChecksumIsValid, so a
		// frame produced here passes the real decode pipeline unmodified.
		static std::vector<uint8_t> CreateValidChecksummedMessage(uint8_t destination, uint8_t command, const std::vector<uint8_t>& payload)
		{
			std::vector<uint8_t> message;
			message.push_back(0x10); // Header DLE
			message.push_back(0x02); // Header STX
			message.push_back(destination);
			message.push_back(command);

			for (auto byte : payload)
			{
				message.push_back(byte);
			}

			// Checksum covers [DLE .. last payload byte] (the whole frame so far).
			const uint8_t checksum = Utility::JandyPacket_CalculateChecksum_FromRange(message);
			message.push_back(checksum);

			message.push_back(0x10); // Footer DLE
			message.push_back(0x03); // Footer ETX

			return message;
		}

		// Retained for backwards compatibility; now produces a genuinely
		// checksummed frame rather than the previous unchecksummed placeholder.
		static std::vector<uint8_t> CreateMessageWithChecksum(uint8_t destination, uint8_t command, const std::vector<uint8_t>& payload)
		{
			return CreateValidChecksummedMessage(destination, command, payload);
		}
	};

	// Builds a fully-framed Pentair RS-485 frame WITH a valid 16-bit checksum.
	//
	// Wire layout:
	//   [0xFF][0x00][0xFF][0xA5][FROM][DEST][CMD][LEN][DATA...][CHK_HI][CHK_LO]
	//
	// The checksum is the 16-bit sum over the checksummed region (the 0xA5 SOF
	// through the last DATA byte), encoded big-endian.  LEN is set automatically
	// from the supplied payload.  A frame produced here passes the real Pentair
	// decode pipeline unmodified.
	class PentairMessageBuilder
	{
	public:
		static std::vector<uint8_t> CreateValidChecksummedFrame(uint8_t from, uint8_t dest, uint8_t command, const std::vector<uint8_t>& payload)
		{
			std::vector<uint8_t> frame;

			// Preamble (excluded from the checksum).
			frame.push_back(0xFF);
			frame.push_back(0x00);
			frame.push_back(0xFF);

			// Checksummed region starts at the 0xA5 SOF.
			const std::size_t region_start = frame.size();
			frame.push_back(0xA5);
			frame.push_back(from);
			frame.push_back(dest);
			frame.push_back(command);
			frame.push_back(static_cast<uint8_t>(payload.size())); // LEN

			for (auto byte : payload)
			{
				frame.push_back(byte);
			}

			// 16-bit big-endian checksum over [0xA5 .. last DATA byte].
			const std::vector<uint8_t> region(frame.begin() + static_cast<std::ptrdiff_t>(region_start), frame.end());
			const uint16_t checksum = Pentair::Utility::PentairPacket_CalculateChecksum_FromRange(region);
			frame.push_back(static_cast<uint8_t>((checksum >> 8) & 0xFF));
			frame.push_back(static_cast<uint8_t>(checksum & 0xFF));

			return frame;
		}
	};

}
// namespace AqualinkAutomate::Test
