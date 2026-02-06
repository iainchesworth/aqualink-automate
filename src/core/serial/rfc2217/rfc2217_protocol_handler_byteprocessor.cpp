#include <cstdint>
#include <optional>

#include "logging/logging.h"
#include "profiling/profiling.h"
#include "serial/rfc2217/rfc2217_protocol_handler.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Serial::RFC2217
{

	std::optional<uint8_t> ProtocolHandler::ProcessIAC(uint8_t byte)
	{
		if (byte == Constants::IAC)
		{
			m_State = State::DATA;
			return byte;  // Escaped IAC
		}

		if (byte == Constants::SB)
		{
			LogTrace(Channel::Serial, "RFC2217 subnegotiation started");
			m_CommandBuffer.clear();
			m_State = State::SUBNEG;
			return std::nullopt;
		}

		if (byte == Constants::WILL || byte == Constants::WONT || byte == Constants::DO || byte == Constants::DONT)
		{
			m_State = State::COMMAND;
			return std::nullopt;
		}

		LogWarning(Channel::Serial, std::format("Unknown RFC2217 command byte: 0x{:02X}", byte));
		m_State = State::DATA;
		return std::nullopt;
	}

	std::optional<uint8_t> ProtocolHandler::ProcessCommand(uint8_t byte)
	{
		if (byte == Constants::COM_PORT_OPTION)
		{
			LogDebug(Channel::Serial, "RFC2217 COM-PORT-OPTION acknowledged by remote");
		}
		else
		{
			LogTrace(Channel::Serial, std::format("RFC2217 command received: 0x{:02X}", byte));
		}

		m_State = State::DATA;
		return std::nullopt;
	}

	std::optional<uint8_t> ProtocolHandler::ProcessSubnegIAC(uint8_t byte)
	{
		if (byte == Constants::SE)
		{
			LogTrace(Channel::Serial, std::format("RFC2217 subnegotiation complete ({} bytes)", m_CommandBuffer.size()));

			if (!m_CommandBuffer.empty() && m_CommandBuffer[0] == Constants::COM_PORT_OPTION)
			{
				if (m_CommandBuffer.size() >= 2) [[likely]]
				{
					const auto command = m_CommandBuffer[1];
					const auto params = std::span{ m_CommandBuffer }.subspan(2);
					HandleCommand(command, params);
				}
				else
				{
					LogWarning(Channel::Serial, "Incomplete RFC2217 subnegotiation command");
				}
			}

			m_State = State::DATA;
			return std::nullopt;
		}

		if (byte == Constants::IAC)
		{
			// Security: Check buffer size before appending
			if (m_CommandBuffer.size() >= MAX_COMMAND_BUFFER_SIZE)
			{
				LogWarning(Channel::Serial, "RFC2217 command buffer overflow attempt blocked");
				m_CommandBuffer.clear();
				m_State = State::DATA;
				return std::nullopt;
			}
			m_CommandBuffer.push_back(byte);
			m_State = State::SUBNEG;
			return std::nullopt;
		}

		LogWarning(Channel::Serial, std::format("Unexpected byte in subnegotiation IAC: 0x{:02X}", byte));
		// Security: Check buffer size before appending
		if (m_CommandBuffer.size() >= MAX_COMMAND_BUFFER_SIZE)
		{
			LogWarning(Channel::Serial, "RFC2217 command buffer overflow attempt blocked");
			m_CommandBuffer.clear();
			m_State = State::DATA;
			return std::nullopt;
		}
		m_CommandBuffer.push_back(byte);
		m_State = State::SUBNEG;
		return std::nullopt;
	}

	std::optional<uint8_t> ProtocolHandler::ProcessByte(uint8_t byte)
	{
		switch (m_State)
		{
		case State::DATA:
			if (byte == Constants::IAC) [[unlikely]]
			{
				m_State = State::IAC;
				return std::nullopt;
			}
			return byte;

		case State::IAC:
			return ProcessIAC(byte);

		case State::COMMAND:
			return ProcessCommand(byte);

		case State::SUBNEG:
			if (byte == Constants::IAC)
			{
				m_State = State::SUBNEG_IAC;
			}
			else
			{
				// Security: Check buffer size before appending
				if (m_CommandBuffer.size() >= MAX_COMMAND_BUFFER_SIZE)
				{
					LogWarning(Channel::Serial, "RFC2217 command buffer overflow attempt blocked");
					m_CommandBuffer.clear();
					m_State = State::DATA;
					return std::nullopt;
				}
				m_CommandBuffer.push_back(byte);
			}
			return std::nullopt;

		case State::SUBNEG_IAC:
			return ProcessSubnegIAC(byte);
		}

		std::unreachable();
	}

}
// namespace AqualinkAutomate::Serial::RFC2217
