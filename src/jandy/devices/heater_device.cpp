#include <format>
#include <functional>

#include <magic_enum/magic_enum.hpp>

#include "devices/heater_device.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices
{
	HeaterDevice::HeaterDevice(const std::shared_ptr<Devices::JandyDeviceType>& device_id) :
		JandyDevice(device_id),
		Capabilities::Restartable(HEATER_TIMEOUT_DURATION),
		m_OperatingMode(std::make_pair(Messages::HeaterOperatingModes::Unknown, std::chrono::system_clock::now())),
		m_PoolSetpoint(std::make_pair(0, std::chrono::system_clock::now())),
		m_SpaSetpoint(std::make_pair(0, std::chrono::system_clock::now())),
		m_WaterTemperature(std::make_pair(0, std::chrono::system_clock::now())),
		m_HeaterState(std::make_pair(Messages::HeaterStates::Unknown, std::chrono::system_clock::now())),
		m_ErrorCode(std::make_pair(Messages::HeaterErrors::Unknown, std::chrono::system_clock::now()))
	{
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::HeaterMessage_Request>([this](auto&& PH1) { Slot_Heater_Request(std::forward<decltype(PH1)>(PH1)); }, (*device_id)());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::HeaterMessage_Status>([this](auto&& PH1) { Slot_Heater_Status(std::forward<decltype(PH1)>(PH1)); }, (*device_id)());
	}

	void HeaterDevice::WatchdogTimeoutOccurred()
	{
	}

	HeaterDevice::TimestampedMode HeaterDevice::OperatingMode() const
	{
		return m_OperatingMode;
	}

	HeaterDevice::TimestampedTemp HeaterDevice::PoolSetpoint() const
	{
		return m_PoolSetpoint;
	}

	HeaterDevice::TimestampedTemp HeaterDevice::SpaSetpoint() const
	{
		return m_SpaSetpoint;
	}

	HeaterDevice::TimestampedTemp HeaterDevice::WaterTemperature() const
	{
		return m_WaterTemperature;
	}

	HeaterDevice::TimestampedState HeaterDevice::HeaterState() const
	{
		return m_HeaterState;
	}

	HeaterDevice::TimestampedError HeaterDevice::ErrorCode() const
	{
		return m_ErrorCode;
	}

	void HeaterDevice::Slot_Heater_Request(const Messages::HeaterMessage_Request& msg)
	{
		LogDebug(Channel::Devices, "Heater device received a HeaterMessage_Request signal.");
		LogDebug(Channel::Devices, std::format("Heater Device: mode={}, pool setpoint={}, spa setpoint={}, water temp={}", magic_enum::enum_name(msg.OperatingMode()), msg.PoolSetpoint(), msg.SpaSetpoint(), msg.WaterTemperature()));

		m_OperatingMode = std::make_pair(msg.OperatingMode(), std::chrono::system_clock::now());
		m_PoolSetpoint = std::make_pair(msg.PoolSetpoint(), std::chrono::system_clock::now());
		m_SpaSetpoint = std::make_pair(msg.SpaSetpoint(), std::chrono::system_clock::now());
		m_WaterTemperature = std::make_pair(msg.WaterTemperature(), std::chrono::system_clock::now());

		// Kick the watchdog to indicate that this device is alive.
		Restartable::Kick();
	}

	void HeaterDevice::Slot_Heater_Status(const Messages::HeaterMessage_Status& msg)
	{
		LogDebug(Channel::Devices, "Heater device received a HeaterMessage_Status signal.");
		LogDebug(Channel::Devices, std::format("Heater Device: state={}, error={}", magic_enum::enum_name(msg.HeaterState()), magic_enum::enum_name(msg.ErrorCode())));

		m_HeaterState = std::make_pair(msg.HeaterState(), std::chrono::system_clock::now());
		m_ErrorCode = std::make_pair(msg.ErrorCode(), std::chrono::system_clock::now());

		// Kick the watchdog to indicate that this device is alive.
		Restartable::Kick();
	}

}
// namespace AqualinkAutomate::Devices
