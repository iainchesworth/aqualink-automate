#include <format>
#include <functional>

#include <magic_enum/magic_enum.hpp>

#include "devices/device_status.h"
#include "logging/logging.h"
#include "devices/serial_adapter_device.h"
#include "utility/overloaded_variant_visitor.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Messages;

namespace AqualinkAutomate::Devices
{

	SerialAdapterDevice::SerialAdapterDevice(const std::shared_ptr<Devices::JandyDeviceType>& device_id, Kernel::HubLocator& hub_locator, bool is_emulated) :
		JandyController(device_id, hub_locator),
		Capabilities::Emulated(is_emulated),
		Capabilities::Restartable(SERIALADAPTER_TIMEOUT_DURATION),
		m_StatusTypesCollection(),
		m_StatusTypesCollectionIter(),
		m_StatusMessageReceived(false),
		m_ProfilingDomain(std::move(Factory::ProfilingUnitFactory::Instance().CreateDomain("SerialAdapterDevice")))
	{
		m_ProfilingDomain->Start();

		auto status_types1{ magic_enum::enum_values<Messages::SerialAdapter_SystemConfigurationStatuses>() };
		auto status_types2{ magic_enum::enum_values<Messages::SerialAdapter_SystemTemperatureCommands>() };
		auto status_types3{ magic_enum::enum_values<AqualinkAutomate::Auxillaries::JandyAuxillaryIds>() };

		m_StatusTypesCollection.insert(m_StatusTypesCollection.end(), std::make_move_iterator(status_types1.begin()), std::make_move_iterator(status_types1.end()));
		m_StatusTypesCollection.insert(m_StatusTypesCollection.end(), std::make_move_iterator(status_types2.begin()), std::make_move_iterator(status_types2.end()));
		m_StatusTypesCollection.insert(m_StatusTypesCollection.end(), std::make_move_iterator(status_types3.begin()), std::make_move_iterator(status_types3.end()));
		m_StatusTypesCollection.erase(m_StatusTypesCollection.begin());  // Don't constantly check the MODEL.

		// Move UNITS to the start of the STC section so temperature unit configuration is
		// known before any temperature/setpoint values are decoded in the first poll cycle.
		auto stc_begin = std::find_if(m_StatusTypesCollection.begin(), m_StatusTypesCollection.end(),
			[](const auto& v) { return std::holds_alternative<SerialAdapter_SystemTemperatureCommands>(v); });
		auto units_it = std::find_if(stc_begin, m_StatusTypesCollection.end(),
			[](const auto& v)
			{
				return std::holds_alternative<SerialAdapter_SystemTemperatureCommands>(v) &&
					std::get<SerialAdapter_SystemTemperatureCommands>(v) == SerialAdapter_SystemTemperatureCommands::UNITS;
			});
		if (units_it != m_StatusTypesCollection.end() && units_it != stc_begin)
		{
			std::rotate(stc_begin, units_it, units_it + 1);
		}

		m_StatusTypesCollectionIter = m_StatusTypesCollection.cbegin();

		m_SlotManager.RegisterSlot_FilterByDeviceId<JandyMessage_Probe>(std::bind(&SerialAdapterDevice::Slot_SerialAdapter_Probe, this, std::placeholders::_1), (*device_id)());
		m_SlotManager.RegisterSlot_FilterByDeviceId<SerialAdapterMessage_DevReady>(std::bind(&SerialAdapterDevice::Slot_SerialAdapter_DevReady, this, std::placeholders::_1), (*device_id)());
		m_SlotManager.RegisterSlot_FilterByDeviceId<SerialAdapterMessage_DevStatus>(std::bind(&SerialAdapterDevice::Slot_SerialAdapter_DevStatus, this, std::placeholders::_1), (*device_id)());
		m_SlotManager.RegisterSlot_FilterByDeviceId<JandyMessage_Status>(std::bind(&SerialAdapterDevice::Slot_SerialAdapter_Status, this, std::placeholders::_1), (*device_id)());
		m_SlotManager.RegisterSlot_FilterByDeviceId<JandyMessage_Unknown>(std::bind(&SerialAdapterDevice::Slot_SerialAdapter_Unknown, this, std::placeholders::_1), (*device_id)());

		if (!IsEmulated())
		{
			m_SlotManager.RegisterSlot_FilterByDeviceId<JandyMessage_Ack>(std::bind(&SerialAdapterDevice::Slot_SerialAdapter_Ack, this, std::placeholders::_1), (*device_id)());
		}

		Status(Devices::DeviceStatus_Initializing{});
	}

	SerialAdapterDevice::~SerialAdapterDevice()
	{
		m_ProfilingDomain->End();
	}

	void SerialAdapterDevice::ProcessControllerUpdates()
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("SerialAdapterDevice::ProcessControllerUpdates", std::source_location::current());

		uint8_t ack_type{ 0x00 };
		uint8_t ack_data_value{ 0x00 };

		if (m_StatusMessageReceived)
		{
			if (m_PendingCommand.has_value())
			{
				auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("SerialAdapterDevice::ProcessControllerUpdates -> pending_command", std::source_location::current());

				LogDebug(Channel::Devices, std::format("ProcessControllerUpdates -> Sending pending command (ack_type=0x{:02x}, ack_data_value=0x{:02x})", m_PendingCommand->ack_type, m_PendingCommand->ack_data_value));

				ack_type = m_PendingCommand->ack_type;
				ack_data_value = m_PendingCommand->ack_data_value;
				m_PendingCommand.reset();
			}
			else
			{
				auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("SerialAdapterDevice::ProcessControllerUpdates -> query_status", std::source_location::current());

				LogDebug(Channel::Devices, "ProcessControllerUpdates -> Query for Status");

				std::visit(
					Utility::OverloadedVisitor
					{
						[](std::monostate)
						{
							LogWarning(Channel::Devices, "SerialAdapterDevice: Cannot select next StatusType; invalid variant content");
						},
						[&ack_type, &ack_data_value](SerialAdapter_ConfigControlCommands sa_ccc)
						{
							LogTrace(Channel::Devices, std::format("SerialAdapterDevice: Selecting Next StatusType -> {} ({:02x})", magic_enum::enum_name(sa_ccc), magic_enum::enum_integer(sa_ccc)));
							ack_type = magic_enum::enum_integer(sa_ccc);
							ack_data_value = static_cast<uint8_t>(Messages::SerialAdapter_CommandTypes::Query);
						},
						[&ack_type, &ack_data_value](SerialAdapter_SystemConfigurationStatuses sa_scs)
						{
							LogTrace(Channel::Devices, std::format("SerialAdapterDevice: Selecting Next StatusType -> {} (0x{:02x})", magic_enum::enum_name(sa_scs), magic_enum::enum_integer(sa_scs)));
							ack_type = magic_enum::enum_integer(sa_scs);
							ack_data_value = static_cast<uint8_t>(Messages::SerialAdapter_CommandTypes::Query);
						},
						[&ack_type, &ack_data_value](SerialAdapter_SystemPumpCommands sa_spc)
						{
							LogTrace(Channel::Devices, std::format("SerialAdapterDevice: Selecting Next StatusType -> {} (0x{:02x})", magic_enum::enum_name(sa_spc), magic_enum::enum_integer(sa_spc)));
							ack_type = magic_enum::enum_integer(sa_spc);
							ack_data_value = static_cast<uint8_t>(Messages::SerialAdapter_CommandTypes::Query);
						},
						[&ack_type, &ack_data_value](SerialAdapter_SystemTemperatureCommands sa_stc)
						{
							LogTrace(Channel::Devices, std::format("SerialAdapterDevice: Selecting Next StatusType -> {} (0x{:02x})", magic_enum::enum_name(sa_stc), magic_enum::enum_integer(sa_stc)));
							ack_type = magic_enum::enum_integer(sa_stc);
							ack_data_value = static_cast<uint8_t>(Messages::SerialAdapter_CommandTypes::Query);
						},
						[&ack_type, &ack_data_value](Auxillaries::JandyAuxillaryIds sa_jai)
						{
							LogTrace(Channel::Devices, std::format("SerialAdapterDevice: Selecting Next StatusType -> {} (0x{:02x})", magic_enum::enum_name(sa_jai), magic_enum::enum_integer(sa_jai) + SerialAdapterMessage_DevStatus::SERIALADAPTER_AUX_ID_OFFSET));
							ack_type = 0x00;
							ack_data_value = magic_enum::enum_integer(sa_jai) + SerialAdapterMessage_DevStatus::SERIALADAPTER_AUX_ID_OFFSET;
						},
						[](SerialAdapter_UnknownCommands sa_uc)
						{
							LogWarning(Channel::Devices, "SerialAdapterDevice: Cannot select next StatusType; unknown command type");
						}
					},
					*m_StatusTypesCollectionIter
				);

				if (m_StatusTypesCollection.cend() == ++m_StatusTypesCollectionIter)
				{
					m_StatusTypesCollectionIter = m_StatusTypesCollection.cbegin();

					if (Devices::DeviceStatus_Initializing{} == Status())
					{
						// Update the status to indicate that the device is now initialised.
						Status(Devices::DeviceStatus_Normal{});
					}
				}
			}

			m_StatusMessageReceived = false;
		}

		Signal_AckMessage(ack_type, ack_data_value);
	}

	void SerialAdapterDevice::QueueCommand(uint8_t ack_type, uint8_t ack_data_value)
	{
		LogDebug(Channel::Devices, std::format("SerialAdapterDevice: Queuing command (ack_type=0x{:02x}, ack_data_value=0x{:02x})", ack_type, ack_data_value));
		m_PendingCommand = PendingCommand{ ack_type, ack_data_value };
	}

	void SerialAdapterDevice::QueuePumpCommand(Messages::SerialAdapter_SystemPumpCommands pump, Messages::SerialAdapter_CommandTypes action)
	{
		LogDebug(Channel::Devices, std::format("SerialAdapterDevice: Queuing pump command ({}, action=0x{:02x})", magic_enum::enum_name(pump), magic_enum::enum_integer(action)));
		QueueCommand(magic_enum::enum_integer(pump), magic_enum::enum_integer(action));
	}

	void SerialAdapterDevice::QueueAuxCommand(Auxillaries::JandyAuxillaryIds aux_id, Messages::SerialAdapter_CommandTypes action)
	{
		LogDebug(Channel::Devices, std::format("SerialAdapterDevice: Queuing aux command ({}, action=0x{:02x})", magic_enum::enum_name(aux_id), magic_enum::enum_integer(action)));
		QueueCommand(magic_enum::enum_integer(aux_id) + Messages::SerialAdapterMessage_DevStatus::SERIALADAPTER_AUX_ID_OFFSET, magic_enum::enum_integer(action));
	}

	void SerialAdapterDevice::QueueSetpointCommand(Messages::SerialAdapter_SystemTemperatureCommands setpoint, uint8_t temperature)
	{
		LogDebug(Channel::Devices, std::format("SerialAdapterDevice: Queuing setpoint command ({}, temperature={})", magic_enum::enum_name(setpoint), temperature));
		QueueCommand(magic_enum::enum_integer(setpoint), temperature);
	}

	void SerialAdapterDevice::WatchdogTimeoutOccurred()
	{
	}

}
// namespace AqualinkAutomate::Devices
