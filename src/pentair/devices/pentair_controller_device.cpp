#include <format>

#include "devices/pentair_controller_device.h"
#include "kernel/temperature.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace Capabilities = AqualinkAutomate::Devices::Capabilities;

namespace AqualinkAutomate::Pentair::Devices
{

	PentairControllerDevice::PentairControllerDevice(const std::shared_ptr<PentairDeviceId>& device_id, Kernel::HubLocator& hub_locator) :
		PentairDevice(device_id),
		Capabilities::Restartable(CONTROLLER_TIMEOUT_DURATION),
		m_Address((nullptr != device_id) ? (*device_id)() : static_cast<uint8_t>(0))
	{
		m_DataHub = hub_locator.Find<Kernel::DataHub>();

		m_SlotManager.RegisterSlot<Messages::PentairControllerMessage_Status>(
			[this](const Messages::PentairControllerMessage_Status& msg) { Slot_Controller_Status(msg); });
	}

	bool PentairControllerDevice::PoolHeaterOn() const
	{
		return m_PoolHeaterOn;
	}

	bool PentairControllerDevice::SpaHeaterOn() const
	{
		return m_SpaHeaterOn;
	}

	void PentairControllerDevice::WatchdogTimeoutOccurred()
	{
		LogWarning(Channel::Devices, std::format("Pentair controller (0x{:02x}) watchdog expired", m_Address));
	}

	void PentairControllerDevice::Slot_Controller_Status(const Messages::PentairControllerMessage_Status& msg)
	{
		// Controller status is broadcast by the controller; match on FROM.
		if (msg.From() != m_Address)
		{
			return;
		}

		LogDebug(Channel::Devices, std::format("Pentair controller (0x{:02x}) status: {:02}:{:02}, water {}F, air {}F, pool-heat {}, spa-heat {}",
			m_Address, msg.Hour(), msg.Minute(), msg.WaterTempF(), msg.AirTempF(), msg.PoolHeaterOn(), msg.SpaHeaterOn()));

		m_PoolHeaterOn = msg.PoolHeaterOn();
		m_SpaHeaterOn = msg.SpaHeaterOn();

		PushStatusToDataHub(msg);

		Capabilities::Restartable::Kick();
	}

	void PentairControllerDevice::PushStatusToDataHub(const Messages::PentairControllerMessage_Status& msg)
	{
		if (!m_DataHub)
		{
			return;
		}

		// The Pentair controller reports temperatures in Fahrenheit.
		m_DataHub->SystemTemperatureUnits(Kernel::TemperatureUnits::Fahrenheit);

		if (msg.HasAirTemp())
		{
			m_DataHub->AirTemp(Kernel::Temperature::ConvertToTemperatureInFahrenheit(static_cast<double>(msg.AirTempF())));
		}

		if (msg.HasWaterTemp())
		{
			// Route the single water-temperature reading to the active body: when
			// the spa circuit is on it is the spa temperature, otherwise the pool.
			const auto water_temp = Kernel::Temperature::ConvertToTemperatureInFahrenheit(static_cast<double>(msg.WaterTempF()));
			if (msg.SpaCircuitOn())
			{
				m_DataHub->SpaTemp(water_temp);
			}
			else
			{
				m_DataHub->PoolTemp(water_temp);
			}
		}

		// Reflect the active circulation mode.
		m_DataHub->CirculationMode = msg.SpaCircuitOn() ? Kernel::CirculationModes::Spa : Kernel::CirculationModes::Pool;
	}

}
// namespace AqualinkAutomate::Pentair::Devices
