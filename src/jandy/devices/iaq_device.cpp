#include <format>
#include <functional>

#include <magic_enum/magic_enum.hpp>

#include "logging/logging.h"
#include "devices/device_status.h"
#include "devices/iaq_device.h"
#include "formatters/jandy_device_formatters.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Devices
{

	IAQDevice::IAQDevice(const std::shared_ptr<Devices::JandyDeviceType>& device_id, Kernel::HubLocator& hub_locator, bool is_emulated) :
		JandyController(device_id, hub_locator),
		Capabilities::Restartable(IAQ_TIMEOUT_DURATION),
		Capabilities::Screen(IAQ_STATUS_PAGE_LINES),
		Capabilities::Emulated(is_emulated),
		m_StatusPage(IAQ_STATUS_PAGE_LINES),
		m_TableInfo(IAQ_MESSAGE_TABLE_LINES),
		m_SM_PageUpdate(m_StatusPage),
		m_SM_TableUpdate(m_TableInfo),
		m_ProfilingDomain(std::move(Factory::ProfilingUnitFactory::Instance().CreateDomain("IAQDevice")))
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::IAQDevice", std::source_location::current());

		LogInfo(Channel::Devices, std::format("Creating IAQDevice: device_id={}, emulated={}, timeout={}s", *device_id, is_emulated, IAQ_TIMEOUT_DURATION.count()));

		m_ProfilingDomain->Start();

		m_SM_PageUpdate.initiate();
		m_SM_TableUpdate.initiate();

		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::IAQMessage_AuxStatus>(std::bind(&IAQDevice::Slot_IAQ_AuxStatus, this, std::placeholders::_1), DeviceId().Id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::IAQMessage_CommandReady>(std::bind(&IAQDevice::Slot_IAQ_CommandReady, this, std::placeholders::_1), DeviceId().Id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::IAQMessage_ControlReady>(std::bind(&IAQDevice::Slot_IAQ_ControlReady, this, std::placeholders::_1), DeviceId().Id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::IAQMessage_Heartbeat>(std::bind(&IAQDevice::Slot_IAQ_Heartbeat, this, std::placeholders::_1), DeviceId().Id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::IAQMessage_MainStatus>(std::bind(&IAQDevice::Slot_IAQ_MainStatus, this, std::placeholders::_1), DeviceId().Id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::IAQMessage_MessageLong>(std::bind(&IAQDevice::Slot_IAQ_MessageLong, this, std::placeholders::_1), DeviceId().Id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::IAQMessage_OneTouchStatus>(std::bind(&IAQDevice::Slot_IAQ_OneTouchStatus, this, std::placeholders::_1), DeviceId().Id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::IAQMessage_PageButton>(std::bind(&IAQDevice::Slot_IAQ_PageButton, this, std::placeholders::_1), DeviceId().Id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::IAQMessage_PageContinue>(std::bind(&IAQDevice::Slot_IAQ_PageContinue, this, std::placeholders::_1), DeviceId().Id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::IAQMessage_PageEnd>(std::bind(&IAQDevice::Slot_IAQ_PageEnd, this, std::placeholders::_1), DeviceId().Id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::IAQMessage_PageMessage>(std::bind(&IAQDevice::Slot_IAQ_PageMessage, this, std::placeholders::_1), DeviceId().Id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::IAQMessage_PageStart>(std::bind(&IAQDevice::Slot_IAQ_PageStart, this, std::placeholders::_1), DeviceId().Id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::IAQMessage_Poll>(std::bind(&IAQDevice::Slot_IAQ_Poll, this, std::placeholders::_1), DeviceId().Id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::IAQMessage_StartUp>(std::bind(&IAQDevice::Slot_IAQ_StartUp, this, std::placeholders::_1), DeviceId().Id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::IAQMessage_TableMessage>(std::bind(&IAQDevice::Slot_IAQ_TableMessage, this, std::placeholders::_1), DeviceId().Id());
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::IAQMessage_TitleMessage>(std::bind(&IAQDevice::Slot_IAQ_TitleMessage, this, std::placeholders::_1), DeviceId().Id());

		LogInfo(Channel::Devices, std::format("IAQ ({}): IAQDevice construction complete - device ready", DeviceId()));
	}

	IAQDevice::~IAQDevice()
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::~IAQDevice", std::source_location::current());

		LogInfo(Channel::Devices, std::format("IAQ ({}): Destroying IAQDevice: final state was {}", DeviceId(), magic_enum::enum_name(m_OpState)));

		m_ProfilingDomain->End();
	}

	void IAQDevice::QueueCommand(uint8_t command)
	{
		LogDebug(Channel::Devices, std::format("IAQ ({}): Queuing command: 0x{:02x}", DeviceId(), command));
		m_PendingCommand = command;
	}

	void IAQDevice::ProcessControllerUpdates()
	{
		ProcessControllerUpdates(false);
	}

	void IAQDevice::ProcessControllerUpdates(bool is_poll_message)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::ProcessControllerUpdates", std::source_location::current());

		LogTrace(Channel::Devices, std::format("IAQ ({}): ProcessControllerUpdates called: state={}, is_poll={}, pending_cmd=0x{:02x}",
			DeviceId(), magic_enum::enum_name(m_OpState), is_poll_message, m_PendingCommand));

		// Non-emulated devices skip straight to NormalOperation on the first update.
		if (!IsEmulated() && m_OpState == OperatingStates::StartUp)
		{
			LogInfo(Channel::Devices, std::format("IAQ ({}): Non-emulated device detected - entering NormalOperation", DeviceId()));
			m_OpState = OperatingStates::NormalOperation;
			Status(Devices::DeviceStatus_Normal{});
		}

		switch (m_OpState)
		{
		case OperatingStates::StartUp:
		{
			LogDebug(Channel::Devices, std::format("IAQ ({}): Processing StartUp state - waiting for MainStatus/AuxStatus", DeviceId()));
			Status(Devices::DeviceStatus_Initializing{});
			break;
		}

		case OperatingStates::NormalOperation:
		{
			LogTrace(Channel::Devices, std::format("IAQ ({}): Processing NormalOperation state", DeviceId()));
			break;
		}

		case OperatingStates::FaultHasOccurred:
		{
			LogWarning(Channel::Devices, std::format("IAQ ({}): Processing FaultHasOccurred state", DeviceId()));
			break;
		}
		}

		// Commands can only be sent in response to IAQ_Poll messages.
		if (is_poll_message && m_PendingCommand != 0x00)
		{
			LogDebug(Channel::Devices, std::format("IAQ ({}): Sending command in Poll ACK: 0x{:02x}", DeviceId(), m_PendingCommand));
			Signal_AckMessage(static_cast<uint8_t>(0x00), m_PendingCommand);
			m_PendingCommand = 0x00;
		}
		else
		{
			Signal_AckMessage(static_cast<uint8_t>(0x00), static_cast<uint8_t>(0x00));
		}
	}

	void IAQDevice::WatchdogTimeoutOccurred()
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::WatchdogTimeoutOccurred", std::source_location::current());

		LogWarning(Channel::Devices, std::format("IAQ ({}): Watchdog timeout occurred: state={}, timeout_duration={}s", DeviceId(), magic_enum::enum_name(m_OpState), IAQ_TIMEOUT_DURATION.count()));

		if (m_OpState == OperatingStates::StartUp)
		{
			LogWarning(Channel::Devices, std::format("IAQ ({}): No valid data received during StartUp -> entering FaultHasOccurred", DeviceId()));
			m_OpState = OperatingStates::FaultHasOccurred;
			Status(Devices::DeviceStatus_FaultOccurred{});
		}
	}

}
// namespace AqualinkAutomate::Devices
