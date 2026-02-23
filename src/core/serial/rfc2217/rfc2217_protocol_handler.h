#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <variant>
#include <vector>

#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "interfaces/iserialportprotocol.h"
#include "serial/serial_port_config.h"
#include "serial/rfc2217/rfc2217_constants.h"
#include "serial/rfc2217/rfc2217_types.h"

namespace AqualinkAutomate::Serial::RFC2217
{
	using SerialisedBuffer = std::variant<boost::asio::const_buffer, std::vector<uint8_t>>;

	class ProtocolHandler : public Interfaces::ISerialPortProtocol
	{
	public:
		explicit ProtocolHandler(boost::asio::ip::tcp::socket& socket);
		~ProtocolHandler() override = default;

		void Initialize() override;
		void Shutdown() override;

		void set_baud_rate(uint32_t baud_rate) override;
		void set_character_size(uint8_t size) override;
		void set_parity(Serial::Parity parity) override;
		void set_stop_bits(Serial::StopBits stop_bits) override;
		void set_flow_control(Serial::FlowControl flow_control) override;

	private:
		void InitiateNegotiation();
		void SendCommand(uint8_t command, std::span<const uint8_t> data = {});
		void HandleCommand(uint8_t command, std::span<const uint8_t> params);

		[[nodiscard]] static constexpr boost::asio::const_buffer ToConstBuffer(const SerialisedBuffer& buffer) noexcept;
		[[nodiscard]] static std::vector<uint8_t> EscapeIACBytes(std::span<const uint8_t> data_to_escape);
		[[nodiscard]] static constexpr std::array<uint8_t, 4> EncodeBaudRate(uint32_t baud_rate) noexcept;

		[[nodiscard]] std::optional<uint8_t> ProcessByte(uint8_t byte);
		[[nodiscard]] std::optional<uint8_t> ProcessCommand(uint8_t byte);
		[[nodiscard]] std::optional<uint8_t> ProcessIAC(uint8_t byte);
		[[nodiscard]] std::optional<uint8_t> ProcessSubnegIAC(uint8_t byte);

	private:
		boost::asio::ip::tcp::socket& m_Socket;

		RFC2217::State m_State{ State::DATA };
		std::vector<uint8_t> m_CommandBuffer;

		// Security: Maximum command buffer size to prevent memory exhaustion from malicious input
		static constexpr std::size_t MAX_COMMAND_BUFFER_SIZE = 4096;
	};

}
// namespace AqualinkAutomate::Serial::RFC2217
