#include "logging/logging.h"
#include "jandy/devices/serial_adapter_device.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Devices
{

	void SerialAdapterDevice::Slot_SerialAdapter_Ack(const Messages::JandyMessage_Ack& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("Serial Adapter MessageProcessors -> Slot_SerialAdapter_Ack", BOOST_CURRENT_LOCATION);
		LogDebug(Channel::Devices, "Serial Adapter device received a JandyMessage_Ack signal.");

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void SerialAdapterDevice::Slot_SerialAdapter_DevReady(const Messages::SerialAdapterMessage_DevReady& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("Serial Adapter MessageProcessors -> Slot_SerialAdapter_DevReady", BOOST_CURRENT_LOCATION);
		LogDebug(Channel::Devices, "Serial Adapter device received a SerialAdapterMessage_DevReady signal.");

		ProcessControllerUpdates();

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void SerialAdapterDevice::Slot_SerialAdapter_DevStatus(const Messages::SerialAdapterMessage_DevStatus& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("Serial Adapter MessageProcessors -> Slot_SerialAdapter_DevStatus", BOOST_CURRENT_LOCATION);
		LogDebug(Channel::Devices, "Serial Adapter device received a SerialAdapterMessage_DevStatus signal.");

		if (msg.Options().has_value() && msg.Options().value().HasCleaner)
		{
			auto it = std::find_if(m_StatusTypesCollection.begin(), m_StatusTypesCollection.end(), 
				[](const auto& v)
				{
					return 
						std::holds_alternative<Messages::SerialAdapter_BasicAuxOperations>(v) && 
						std::get<Messages::SerialAdapter_BasicAuxOperations>(v) == Messages::SerialAdapter_BasicAuxOperations::AUX1;
				}
			);

			if (m_StatusTypesCollection.end() != it)
			{
				// AUX1 is configured as the CLEANER -> checking AUX1 ON/OFF will error so check the "CLEANR" instead.
				*it = Messages::SerialAdapter_SystemPumpCommands::CLEANR;
			}
		}
		
		if (msg.Options().has_value() && msg.Options().value().HasSpillover)
		{
			auto it = std::find_if(m_StatusTypesCollection.begin(), m_StatusTypesCollection.end(),
				[](const auto& v)
				{
					return
						std::holds_alternative<Messages::SerialAdapter_BasicAuxOperations>(v) &&
						std::get<Messages::SerialAdapter_BasicAuxOperations>(v) == Messages::SerialAdapter_BasicAuxOperations::AUX3;
				}
			);

			if (m_StatusTypesCollection.end() != it)
			{
				// AUX3 is configured as the SPA SPILLOVER -> checking AUX3 ON/OFF will error so check the "SPILLOVER" instead.
				*it = Messages::SerialAdapter_SystemPumpCommands::SPILLOVER;
			}
		}

		if (msg.Options().has_value())
		{
			m_StatusTypesCollection.erase(m_StatusTypesCollection.begin()); // Don't constantly check the OPTIONS.
			m_StatusTypesCollectionIter = m_StatusTypesCollection.cbegin(); // Iterators were invalidated; reset to OPMODE.
		}

		ProcessControllerUpdates();

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void SerialAdapterDevice::Slot_SerialAdapter_Probe(const Messages::JandyMessage_Probe& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("Serial Adapter MessageProcessors -> Slot_SerialAdapter_Probe", BOOST_CURRENT_LOCATION);
		LogDebug(Channel::Devices, "Serial Adapter device received a JandyMessage_Probe signal.");

		ProcessControllerUpdates();

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void SerialAdapterDevice::Slot_SerialAdapter_Status(const Messages::JandyMessage_Status& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("Serial Adapter MessageProcessors -> Slot_SerialAdapter_Status", BOOST_CURRENT_LOCATION);
		LogDebug(Channel::Devices, "Serial Adapter device received a JandyMessage_Status signal.");

		m_StatusMessageReceived = true;

		ProcessControllerUpdates();

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

	void SerialAdapterDevice::Slot_SerialAdapter_Unknown(const Messages::JandyMessage_Unknown& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("Serial Adapter MessageProcessors -> Slot_SerialAdapter_Unknown", BOOST_CURRENT_LOCATION);
		LogDebug(Channel::Devices, std::format("Serial Adapter device received a JandyMessage_Unknown signal: type -> 0x{:02x}", static_cast<uint8_t>(msg.Id())));

		ProcessControllerUpdates();

		// Kick the watchdog to indicate that this device is alive.
		IDevice::KickTimeoutWatchdog();
	}

}
// namespace AqualinkAutomate::Devices
