#include <cstdint>
#include <format>

#include <magic_enum/magic_enum.hpp>

#include "messages/iaq/iaq_message_main_status.h"
#include "messages/jandy_message_ids.h"
#include "messages/jandy_message_text_helpers.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	namespace
	{
		// Some controller firmware versions append a two-byte sentinel pair after the
		// device-ID list to mark the start of the status fields (legacy format).
		constexpr uint8_t SENTINEL_BYTE_FIRST{ 0x0e };
		constexpr uint8_t SENTINEL_BYTE_SECOND{ 0x0f };

		// Convert raw MainStatus heater byte to HeaterStatuses enum.
		// Protocol values: 0x00=off, 0x01=heating, 0x03=enabled (standby).
		Kernel::HeaterStatuses RawToHeaterStatus(uint8_t raw)
		{
			switch (raw)
			{
			case 0x00: return Kernel::HeaterStatuses::Off;
			case 0x01: return Kernel::HeaterStatuses::Heating;
			case 0x03: return Kernel::HeaterStatuses::Enabled;
			default:   return Kernel::HeaterStatuses::Unknown;
			}
		}
	}
	// anonymous namespace

	IAQMessage_MainStatus::IAQMessage_MainStatus() noexcept :
		IAQMessage(JandyMessageIds::IAQ_MainStatus),
		Interfaces::IMessageSignalRecv<IAQMessage_MainStatus>()
	{
	}


	const std::vector<uint8_t>& IAQMessage_MainStatus::RawPayload() const
	{
		return m_RawPayload;
	}

	bool IAQMessage_MainStatus::PumpOn() const
	{
		return m_PumpOn;
	}

	bool IAQMessage_MainStatus::SpaMode() const
	{
		return m_SpaMode;
	}

	Kernel::Temperature IAQMessage_MainStatus::PoolTemperature() const
	{
		return m_PoolTemp;
	}

	Kernel::Temperature IAQMessage_MainStatus::SpaTemperature() const
	{
		return m_SpaTemp;
	}

	Kernel::Temperature IAQMessage_MainStatus::AirTemperature() const
	{
		return m_AirTemp;
	}

	std::optional<Kernel::Temperature> IAQMessage_MainStatus::HeaterSetpoint() const
	{
		return m_HeaterSetpoint;
	}

	Kernel::HeaterStatuses IAQMessage_MainStatus::PoolHeaterStatus() const
	{
		return m_PoolHeaterStatus;
	}

	Kernel::HeaterStatuses IAQMessage_MainStatus::SpaHeaterStatus() const
	{
		return m_SpaHeaterStatus;
	}

	Kernel::HeaterStatuses IAQMessage_MainStatus::SolarHeaterStatus() const
	{
		return m_SolarHeaterStatus;
	}

	const std::vector<uint8_t>& IAQMessage_MainStatus::DeviceIds() const
	{
		return m_DeviceIds;
	}

	std::string IAQMessage_MainStatus::ToString() const
	{
		std::string heater_str;
		if (m_HeaterSetpoint.has_value())
		{
			heater_str = std::format(", Heater Setpoint: {:.1f}C", m_HeaterSetpoint->InCelsius().value());
		}

		return std::format(
			"Packet: {} || Pump: {}, Spa Mode: {}, Pool: {:.1f}C, Spa: {:.1f}C, Air: {:.1f}C, PoolHeat: {}, SpaHeat: {}, Solar: {}{}",
			IAQMessage::ToString(),
			m_PumpOn ? "ON" : "OFF",
			m_SpaMode ? "ON" : "OFF",
			m_PoolTemp.InCelsius().value(),
			m_SpaTemp.InCelsius().value(),
			m_AirTemp.InCelsius().value(),
			magic_enum::enum_name(m_PoolHeaterStatus),
			magic_enum::enum_name(m_SpaHeaterStatus),
			magic_enum::enum_name(m_SolarHeaterStatus),
			heater_str
		);
	}

	bool IAQMessage_MainStatus::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		return false;
	}

	bool IAQMessage_MainStatus::DeserializeContents(std::span<const uint8_t> message_bytes)
	{
		LogTrace(Channel::Messages, [&]() { return std::format("Deserialising {} bytes from span into IAQMessage_MainStatus type", message_bytes.size()); });

		if (message_bytes.size() <= JandyMessage::PACKET_HEADER_LENGTH + JandyMessage::PACKET_FOOTER_LENGTH)
		{
			LogDebug(Channel::Messages, "IAQMessage_MainStatus is too short to contain any payload.");
			return false;
		}

		// Extract raw payload (between the header and the footer) in a single allocation.
		const std::size_t payload_length = message_bytes.size() - JandyMessage::PACKET_HEADER_LENGTH - JandyMessage::PACKET_FOOTER_LENGTH;
		m_RawPayload.reserve(payload_length);
		m_RawPayload.assign(
			message_bytes.begin() + JandyMessage::PACKET_HEADER_LENGTH,
			message_bytes.end() - JandyMessage::PACKET_FOOTER_LENGTH);

		const auto& payload = m_RawPayload;
		const size_t payload_size = payload.size();
		size_t pos = 0;

		// Byte 0: device count — number of device IDs that follow.
		if (pos >= payload_size)
		{
			LogDebug(Channel::Messages, "IAQMessage_MainStatus payload too short for device count.");
			return false;
		}

		const uint8_t device_count = payload[pos++];

		// Read exactly device_count device IDs.
		// NOTE: 0x0e and 0x0f can be valid device IDs on larger panels (e.g. RS-8 Combo),
		// so we cannot use them as sentinel markers to find the end of the device list.
		m_DeviceIds.clear();

		if (pos + device_count > payload_size)
		{
			LogDebug(Channel::Messages, [device_count]() { return std::format("IAQMessage_MainStatus payload too short for {} device IDs.", device_count); });
			return false;
		}

		m_DeviceIds.reserve(device_count);
		for (uint8_t i = 0; i < device_count; ++i)
		{
			m_DeviceIds.push_back(payload[pos++]);
		}

		// Some controller firmware versions append a sentinel pair after the device
		// IDs. If present, skip it so we reach the status fields correctly.
		const bool has_sentinel = (pos + 1 < payload_size && payload[pos] == SENTINEL_BYTE_FIRST && payload[pos + 1] == SENTINEL_BYTE_SECOND);
		if (has_sentinel)
		{
			pos += 2;
		}

		LogTrace(Channel::Messages, [device_count, has_sentinel, pos]() { return std::format("IAQMessage_MainStatus: device_count={}, sentinel={}, status_offset={}",
			device_count, has_sentinel, pos); });

		// Read temperature helper — big-endian uint16.
		auto read_temp_be = [&payload, &pos](double scale) -> Kernel::Temperature
		{
			uint16_t raw = (static_cast<uint16_t>(payload[pos]) << 8) | static_cast<uint16_t>(payload[pos + 1]);
			pos += 2;
			return Kernel::Temperature::ConvertToTemperatureInCelsius(raw * scale);
		};

		if (has_sentinel)
		{
			// Legacy format (smaller panels with sentinel):
			//   pump(1), spa_mode(1), unknown(2), temps in BE deci-Celsius (÷10)
			if (pos + 4 > payload_size)
			{
				LogDebug(Channel::Messages, "IAQMessage_MainStatus payload too short for legacy status flags.");
				return false;
			}

			m_PumpOn = (payload[pos++] != 0x00);
			m_SpaMode = (payload[pos++] != 0x00);
			pos += 2; // Skip 2 unknown bytes

			if (pos + 6 > payload_size)
			{
				LogDebug(Channel::Messages, "IAQMessage_MainStatus payload too short for legacy temperature data.");
				return false;
			}

			m_PoolTemp = read_temp_be(0.1);
			m_SpaTemp = read_temp_be(0.1);
			m_AirTemp = read_temp_be(0.1);

			// In legacy format, extra bytes after the three temperatures
			// are the heater setpoint (when a heater device ID is present).
			if (pos + 2 <= payload_size)
			{
				m_HeaterSetpoint = read_temp_be(0.1);
			}
			else
			{
				m_HeaterSetpoint = std::nullopt;
			}
		}
		else
		{
			// Current format (larger panels, e.g. RS-8 Combo):
			//   pump(1), unk(1), spa_mode(1), unk(1), flags(1),
			//   pool_target(2 BE °C), spa_target(2 BE °C), air(2 BE °C), water_current(2 BE °C)
			if (pos + 5 > payload_size)
			{
				LogDebug(Channel::Messages, "IAQMessage_MainStatus payload too short for status flags.");
				return false;
			}

			m_PumpOn = (payload[pos++] != 0x00);
			m_PoolHeaterStatus = RawToHeaterStatus(payload[pos++]);
			m_SpaMode = (payload[pos++] != 0x00);
			m_SpaHeaterStatus = RawToHeaterStatus(payload[pos++]);
			m_SolarHeaterStatus = RawToHeaterStatus(payload[pos++]);

			if (pos + 8 > payload_size)
			{
				LogDebug(Channel::Messages, "IAQMessage_MainStatus payload too short for temperature data.");
				return false;
			}

			auto pool_target = read_temp_be(1.0);
			auto spa_target = read_temp_be(1.0);
			m_AirTemp = read_temp_be(1.0);
			auto water_current = read_temp_be(1.0);

			m_PoolTemp = pool_target;
			m_SpaTemp = water_current;
			m_HeaterSetpoint = m_SpaMode ? spa_target : pool_target;
		}

		LogDebug(Channel::Messages, [this]() { return std::format(
			"Deserialised IAQMessage_MainStatus: Pump={}, SpaMode={}, Pool={:.1f}C, Spa={:.1f}C, Air={:.1f}C, PoolHeat={}, SpaHeat={}, Solar={}",
			m_PumpOn, m_SpaMode,
			m_PoolTemp.InCelsius().value(),
			m_SpaTemp.InCelsius().value(),
			m_AirTemp.InCelsius().value(),
			magic_enum::enum_name(m_PoolHeaterStatus),
			magic_enum::enum_name(m_SpaHeaterStatus),
			magic_enum::enum_name(m_SolarHeaterStatus)
		); });

		return true;
	}

}
// namespace AqualinkAutomate::Messages
