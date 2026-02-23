#pragma once

#include <cstdint>
#include <cstddef>
#include <optional>
#include <string>
#include <span>
#include <vector>

#include "interfaces/imessagesignal_recv.h"
#include "kernel/auxillary_devices/heater_status.h"
#include "kernel/temperature.h"
#include "messages/iaq/iaq_message.h"

namespace AqualinkAutomate::Messages
{

	class IAQMessage_MainStatus : public IAQMessage, public Interfaces::IMessageSignalRecv<IAQMessage_MainStatus>
	{
	public:
		IAQMessage_MainStatus() noexcept;
		~IAQMessage_MainStatus() override = default;

	public:
		const std::vector<uint8_t>& RawPayload() const;
		bool PumpOn() const;
		bool SpaMode() const;
		Kernel::Temperature PoolTemperature() const;
		Kernel::Temperature SpaTemperature() const;
		Kernel::Temperature AirTemperature() const;
		std::optional<Kernel::Temperature> HeaterSetpoint() const;
		Kernel::HeaterStatuses PoolHeaterStatus() const;
		Kernel::HeaterStatuses SpaHeaterStatus() const;
		Kernel::HeaterStatuses SolarHeaterStatus() const;
		const std::vector<uint8_t>& DeviceIds() const;

	public:
		std::string ToString() const override;

	public:
		bool SerializeContents(std::vector<uint8_t>& message_bytes) const override;
		bool DeserializeContents(std::span<const uint8_t> message_bytes) override;

	private:
		std::vector<uint8_t> m_RawPayload;
		bool m_PumpOn{false};
		bool m_SpaMode{false};
		Kernel::Temperature m_PoolTemp{Kernel::Temperature::ConvertToTemperatureInCelsius(0.0)};
		Kernel::Temperature m_SpaTemp{Kernel::Temperature::ConvertToTemperatureInCelsius(0.0)};
		Kernel::Temperature m_AirTemp{Kernel::Temperature::ConvertToTemperatureInCelsius(0.0)};
		std::optional<Kernel::Temperature> m_HeaterSetpoint;
		Kernel::HeaterStatuses m_PoolHeaterStatus{Kernel::HeaterStatuses::Unknown};
		Kernel::HeaterStatuses m_SpaHeaterStatus{Kernel::HeaterStatuses::Unknown};
		Kernel::HeaterStatuses m_SolarHeaterStatus{Kernel::HeaterStatuses::Unknown};
		std::vector<uint8_t> m_DeviceIds;
	};

}
// namespace AqualinkAutomate::Messages
