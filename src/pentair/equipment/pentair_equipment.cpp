#include <format>
#include <functional>

#include "devices/pentair_chlorinator_device.h"
#include "devices/pentair_controller_device.h"
#include "devices/pentair_device_id.h"
#include "devices/pentair_vsp_pump_device.h"
#include "equipment/equipment_status.h"
#include "equipment/pentair_equipment.h"
#include "messages/pentair_message_constants.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Pentair::Equipment
{

	PentairEquipment::PentairEquipment(Kernel::HubLocator& hub_locator) :
		IEquipment(),
		IStatusPublisher(AqualinkAutomate::Equipment::EquipmentStatus_Unknown{}),
		m_MessageConnections(),
		m_HubLocator(hub_locator)
	{
		m_DataHub = m_HubLocator.Find<Kernel::DataHub>();
		m_EquipmentHub = m_HubLocator.Find<Kernel::EquipmentHub>();
		m_StatsHub = m_HubLocator.Find<Kernel::StatisticsHub>();

		m_MessageConnections.push_back(
			Messages::PentairPumpMessage_Status::GetSignal()->connect(
				std::bind(&PentairEquipment::IdentifyAndAddPump, this, std::placeholders::_1)));

		m_MessageConnections.push_back(
			Messages::PentairChlorinatorMessage_Status::GetSignal()->connect(
				std::bind(&PentairEquipment::IdentifyAndAddChlorinator, this, std::placeholders::_1)));

		m_MessageConnections.push_back(
			Messages::PentairControllerMessage_Status::GetSignal()->connect(
				std::bind(&PentairEquipment::IdentifyAndAddController, this, std::placeholders::_1)));
	}

	PentairEquipment::~PentairEquipment()
	{
		for (auto& connection : m_MessageConnections)
		{
			connection.disconnect();
		}
	}

	void PentairEquipment::IdentifyAndAddPump(const Messages::PentairPumpMessage_Status& message)
	{
		if (nullptr == m_EquipmentHub)
		{
			return;
		}

		// Status frames are broadcast by the pump, so FROM is the pump address.
		const uint8_t address = message.From();

		const bool in_pump_range = (Messages::PUMP_ADDRESS_BASE <= address) && (address <= Messages::PUMP_ADDRESS_LAST);
		if (!in_pump_range)
		{
			return;
		}

		Devices::PentairDeviceId candidate_id(address);
		if (m_EquipmentHub->DeviceExists(candidate_id))
		{
			return;
		}

		LogInfo(Channel::Equipment, std::format("Adding new Pentair VSP pump device at address 0x{:02x}", address));

		auto device_id = std::make_shared<Devices::PentairDeviceId>(address);
		m_EquipmentHub->AddDevice(std::make_unique<Devices::PentairVSPPumpDevice>(device_id, m_HubLocator));
	}

	void PentairEquipment::IdentifyAndAddChlorinator(const Messages::PentairChlorinatorMessage_Status& message)
	{
		if (nullptr == m_EquipmentHub)
		{
			return;
		}

		const uint8_t address = message.From();

		Devices::PentairDeviceId candidate_id(address);
		if (m_EquipmentHub->DeviceExists(candidate_id))
		{
			return;
		}

		LogInfo(Channel::Equipment, std::format("Adding new Pentair IntelliChlor device at address 0x{:02x}", address));

		auto device_id = std::make_shared<Devices::PentairDeviceId>(address);
		m_EquipmentHub->AddDevice(std::make_unique<Devices::PentairChlorinatorDevice>(device_id, m_HubLocator));
	}

	void PentairEquipment::IdentifyAndAddController(const Messages::PentairControllerMessage_Status& message)
	{
		if (nullptr == m_EquipmentHub)
		{
			return;
		}

		const uint8_t address = message.From();

		Devices::PentairDeviceId candidate_id(address);
		if (m_EquipmentHub->DeviceExists(candidate_id))
		{
			return;
		}

		LogInfo(Channel::Equipment, std::format("Adding new Pentair controller device at address 0x{:02x}", address));

		auto device_id = std::make_shared<Devices::PentairDeviceId>(address);
		m_EquipmentHub->AddDevice(std::make_unique<Devices::PentairControllerDevice>(device_id, m_HubLocator));
	}

}
// namespace AqualinkAutomate::Pentair::Equipment
