#pragma once

#include <cstddef>
#include <cstdint>
#include <format>
#include <source_location>
#include <span>
#include <string>
#include <string_view>

#include "messages/jandy_message.h"
#include "logging/logging.h"

namespace AqualinkAutomate::Messages::Text
{
	//
	// Shared text-extraction / sanitisation / scalar-read helpers for the Jandy and
	// IAQ message decoders.
	//
	// These centralise three idioms that were previously copy-pasted across ~24
	// decoders:
	//
	//   * the "is the packet long enough to read field X" bounds-check + LogDebug,
	//   * the trailing-ASCII payload extraction (header .. footer) with its
	//     easy-to-get-wrong integer-underflow guard, and
	//   * the little/big-endian scalar reads.
	//
	// They are header-only inline free functions so they require no addition to
	// src/jandy/CMakeLists.txt (that file is owned by a separate work unit).
	//

	//
	// Map a single wire byte to a printable ASCII character.  Wire-derived strings
	// (page/title/line text, device-id, aux device names) are copied into message
	// fields and then surfaced into logs, the web UI and MQTT.  A malformed or
	// hostile frame can carry control bytes (NUL, ESC, CR/LF), terminal escape
	// sequences, or high-bit bytes; passing those through unsanitised risks log
	// injection / terminal-escape attacks and corrupt UI rendering.  Anything
	// outside the printable range [0x20, 0x7E] is replaced with '?'.
	//
	[[nodiscard]] constexpr char SanitisePrintableAsciiByte(uint8_t byte) noexcept
	{
		constexpr uint8_t FIRST_PRINTABLE_ASCII{ 0x20 }; // space
		constexpr uint8_t LAST_PRINTABLE_ASCII{ 0x7E };  // tilde

		if (byte >= FIRST_PRINTABLE_ASCII && byte <= LAST_PRINTABLE_ASCII)
		{
			return static_cast<char>(byte);
		}

		return '?';
	}

	//
	// Append a span of wire bytes onto an output string, sanitising each byte to
	// printable ASCII (see SanitisePrintableAsciiByte).
	//
	inline void AppendSanitisedAscii(std::string& out, std::span<const uint8_t> bytes)
	{
		out.reserve(out.size() + bytes.size());

		for (const uint8_t byte : bytes)
		{
			out.push_back(SanitisePrintableAsciiByte(byte));
		}
	}

	//
	// Extract the trailing ASCII payload from a fully-framed packet: the bytes
	// from start_index up to (but not including) the JandyMessage::PACKET_FOOTER_LENGTH
	// trailing footer (checksum + DLE + ETX), sanitised to printable ASCII.
	//
	// The underflow guard is centralised here: if the packet is too short to hold
	// any payload between start_index and the footer, an empty string is returned
	// and no out-of-range / wrap-around length is ever computed.
	//
	[[nodiscard]] inline std::string ExtractTrailingAsciiPayload(std::span<const uint8_t> message_bytes, std::size_t start_index)
	{
		const std::size_t footer = JandyMessage::PACKET_FOOTER_LENGTH;

		// Guard against integer underflow: there must be at least start_index +
		// footer + 1 byte present for a single payload character to exist.
		if (message_bytes.size() <= start_index + footer)
		{
			return std::string{};
		}

		const std::size_t length_to_copy = message_bytes.size() - start_index - footer;

		std::string result;
		AppendSanitisedAscii(result, message_bytes.subspan(start_index, length_to_copy));

		return result;
	}

	//
	// Bounds guard shared by the "too short to deserialise field X" idiom.  Returns
	// true when message_bytes is long enough to index 'required_index' (i.e. when
	// size > required_index), otherwise emits the standard LogDebug line and returns
	// false.  Callers fall through to a `return false` on a false result.
	//
	[[nodiscard]] inline bool RequireIndex(std::span<const uint8_t> message_bytes, std::size_t required_index, std::string_view type_name, std::string_view field_name)
	{
		if (message_bytes.size() > required_index)
		{
			return true;
		}

		Logging::Log(
			[type_name, field_name]() { return std::format("{} is too short to deserialise {}.", type_name, field_name); },
			Logging::Channel::Messages,
			Logging::Severity::Debug,
			std::source_location::current());

		return false;
	}

	//
	// Read a single unsigned byte at 'index'.  The caller must have verified the
	// span is long enough (e.g. via RequireIndex).
	//
	[[nodiscard]] constexpr uint8_t ReadU8(std::span<const uint8_t> message_bytes, std::size_t index)
	{
		return message_bytes[index];
	}

	//
	// Read a little-endian uint16 from [low_index, low_index+1].  The caller must
	// have verified the span is long enough (e.g. via RequireIndex on the high
	// byte index).
	//
	[[nodiscard]] constexpr uint16_t ReadU16LE(std::span<const uint8_t> message_bytes, std::size_t low_index)
	{
		const uint16_t low = message_bytes[low_index];
		const uint16_t high = message_bytes[low_index + 1U];

		return static_cast<uint16_t>((high << 8U) | low);
	}

}
// namespace AqualinkAutomate::Messages::Text
