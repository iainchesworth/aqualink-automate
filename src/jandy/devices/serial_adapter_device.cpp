#include <format>
#include <functional>

#include "logging/logging.h"
#include "jandy/devices/serial_adapter_device.h"
#include "utility/overloaded_variant_visitor.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Messages;

namespace AqualinkAutomate::Devices
{

	SerialAdapterDevice::SerialAdapterDevice(boost::asio::io_context& io_context, std::shared_ptr<Devices::JandyDeviceType> device_id, Kernel::DataHub& config, bool is_emulated) :
		JandyController(io_context, device_id, SERIALADAPTER_TIMEOUT_DURATION, config),
		Capabilities::Emulated(is_emulated),
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
	}

	SerialAdapterDevice::~SerialAdapterDevice()
	{
		m_ProfilingDomain->End();
	}

	void SerialAdapterDevice::ProcessControllerUpdates()
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("ProcessControllerUpdates -> Handle Operating State", BOOST_CURRENT_LOCATION);

		uint8_t ack_type{ 0x00 };
		uint8_t ack_data_value{ 0x00 };

		if (m_StatusMessageReceived)
		{
			auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("ProcessControllerUpdates -> Query for Status", BOOST_CURRENT_LOCATION);
			
			LogDebug(Channel::Messages, "ProcessControllerUpdates -> Query for Status");

			std::visit(
				Utility::OverloadedVisitor
				{
					[](std::monostate)
					{
						LogWarning(Channel::Messages, "SerialAdapterDevice: Cannot select next StatusType; invalid variant content");
					},
					[&ack_type, &ack_data_value](SerialAdapter_ConfigControlCommands sa_ccc)
					{
						LogTrace(Channel::Messages, std::format("SerialAdapterDevice: Selecting Next StatusType -> {} ({:02x})", magic_enum::enum_name(sa_ccc), magic_enum::enum_integer(sa_ccc)));
						ack_type = magic_enum::enum_integer(sa_ccc);
						ack_data_value = static_cast<uint8_t>(Messages::SerialAdapter_CommandTypes::Query);
					},
					[&ack_type, &ack_data_value](SerialAdapter_SystemConfigurationStatuses sa_scs)
					{
						LogTrace(Channel::Messages, std::format("SerialAdapterDevice: Selecting Next StatusType -> {} (0x{:02x})", magic_enum::enum_name(sa_scs), magic_enum::enum_integer(sa_scs)));
						ack_type = magic_enum::enum_integer(sa_scs);
						ack_data_value = static_cast<uint8_t>(Messages::SerialAdapter_CommandTypes::Query);
					},
					[&ack_type, &ack_data_value](SerialAdapter_SystemPumpCommands sa_spc)
					{
						LogTrace(Channel::Messages, std::format("SerialAdapterDevice: Selecting Next StatusType -> {} (0x{:02x})", magic_enum::enum_name(sa_spc), magic_enum::enum_integer(sa_spc)));
						ack_type = magic_enum::enum_integer(sa_spc);
						ack_data_value = static_cast<uint8_t>(Messages::SerialAdapter_CommandTypes::Query);
					},
					[&ack_type, &ack_data_value](SerialAdapter_SystemTemperatureCommands sa_stc)
					{
						LogTrace(Channel::Messages, std::format("SerialAdapterDevice: Selecting Next StatusType -> {} (0x{:02x})", magic_enum::enum_name(sa_stc), magic_enum::enum_integer(sa_stc)));
						ack_type = magic_enum::enum_integer(sa_stc);
						ack_data_value = static_cast<uint8_t>(Messages::SerialAdapter_CommandTypes::Query);
					},
					[&ack_type, &ack_data_value](Auxillaries::JandyAuxillaryIds sa_jai)
					{
						LogTrace(Channel::Messages, std::format("SerialAdapterDevice: Selecting Next StatusType -> {} (0x{:02x})", magic_enum::enum_name(sa_jai), magic_enum::enum_integer(sa_jai) + SerialAdapterMessage_DevStatus::SERIALADAPTER_AUX_ID_OFFSET));
						ack_type = 0x00;
						ack_data_value = magic_enum::enum_integer(sa_jai) + SerialAdapterMessage_DevStatus::SERIALADAPTER_AUX_ID_OFFSET;
					},
					[](SerialAdapter_UnknownCommands sa_uc)
					{
						LogWarning(Channel::Messages, "SerialAdapterDevice: Cannot select next StatusType; unknown command type");
					}
				},
				*m_StatusTypesCollectionIter
			);

			if (m_StatusTypesCollection.cend() == ++m_StatusTypesCollectionIter)
			{
				m_StatusTypesCollectionIter = m_StatusTypesCollection.cbegin();
			}

			m_StatusMessageReceived = false;
		}

		Signal_AckMessage(ack_type, ack_data_value);
	}
}
// namespace AqualinkAutomate::Devices
