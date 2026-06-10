#include <format>
#include <functional>

#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>

#include "logging/logging.h"
#include "devices/device_status.h"
#include "devices/iaq_device.h"
#include "formatters/jandy_device_formatters.h"
#include "messages/iaq/iaq_message_control_data_response.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Devices
{

	namespace
	{
		// IAQ (AqualinkTouch 0x33) UI navigation / chlorinator command bytes.  These ride
		// the 0x33 ACK channel as documented in iaq_protocol.md; named here so the queue
		// builders below are self-describing rather than a string of bare hex literals.
		constexpr uint8_t IAQ_CMD_BACK{ 0x02 };                 // navigate back / clean state
		constexpr uint8_t IAQ_CMD_OPEN_AQUAPURE_PAGE{ 0x19 };   // open the AquaPure settings page
		constexpr uint8_t IAQ_CMD_SELECT_POOL{ 0x11 };          // select Pool (button index 0)
		constexpr uint8_t IAQ_CMD_QUICK_BOOST{ 0x13 };          // Quick Boost (button index 2) / Stop
		constexpr uint8_t IAQ_CMD_BOOST_START{ 0x12 };          // Start boost (button index 1)
		constexpr uint8_t IAQ_CMD_SUBMIT_VALUE{ 0x80 };         // submit the entered value

		constexpr char IAQ_BUTTON_INDEX_POOL{ '1' };            // ASCII Pool button index prefix for control-data values
	}
	// namespace

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
		m_SlotManager.RegisterSlot_FilterByDeviceId<Messages::JandyMessage_Probe>(std::bind(&IAQDevice::Slot_IAQ_Probe, this, std::placeholders::_1), DeviceId().Id());
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
		LogDebug(Channel::Devices, [this, command]() { return std::format("IAQ ({}): Queuing command: 0x{:02x}", DeviceId(), command); });
		m_PendingCommand = command;
	}

	void IAQDevice::QueueChlorinatorPercentage(uint8_t percentage)
	{
		LogInfo(Channel::Devices, [this, percentage]() { return std::format("IAQ ({}): QueueChlorinatorPercentage({}%) - queuing command sequence", DeviceId(), percentage); });

		m_CommandQueue.clear();
		m_CommandQueue.push_back(IAQ_CMD_BACK);
		m_CommandQueue.push_back(IAQ_CMD_OPEN_AQUAPURE_PAGE);
		m_CommandQueue.push_back(IAQ_CMD_SELECT_POOL);
		m_CommandQueue.push_back(IAQ_CMD_SUBMIT_VALUE);

		m_AwaitingControlReady = true;
		m_ControlDataValue = std::format("{}{}", IAQ_BUTTON_INDEX_POOL, percentage);
	}

	void IAQDevice::QueueChlorinatorBoost(bool enable)
	{
		LogInfo(Channel::Devices, [this, enable]() { return std::format("IAQ ({}): QueueChlorinatorBoost({}) - queuing command sequence", DeviceId(), enable); });

		m_CommandQueue.clear();
		m_CommandQueue.push_back(IAQ_CMD_BACK);
		m_CommandQueue.push_back(IAQ_CMD_OPEN_AQUAPURE_PAGE);
		m_CommandQueue.push_back(IAQ_CMD_QUICK_BOOST);

		if (enable)
		{
			m_CommandQueue.push_back(IAQ_CMD_BOOST_START);
		}
		else
		{
			m_CommandQueue.push_back(IAQ_CMD_QUICK_BOOST);  // Stop
		}

		m_AwaitingControlReady = false;
		m_ControlDataValue.clear();
	}

	void IAQDevice::ProcessControllerUpdates()
	{
		ProcessControllerUpdates(false);
	}

	void IAQDevice::ProcessControllerUpdates(bool is_poll_message)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::ProcessControllerUpdates", std::source_location::current());

		LogTrace(Channel::Devices, [&]() { return std::format("IAQ ({}): ProcessControllerUpdates called: state={}, is_poll={}, pending_cmd=0x{:02x}",
			DeviceId(), magic_enum::enum_name(m_OpState), is_poll_message, m_PendingCommand); });

		// This id has been addressed on the bus (poll/status), so any later watchdog
		// timeout is a genuine drop-out rather than "never present".
		m_HasReceivedData = true;

		// Non-emulated devices skip straight to NormalOperation on the first update.
		if (!IsEmulated() && m_OpState == OperatingStates::StartUp)
		{
			LogInfo(Channel::Devices, [this]() { return std::format("IAQ ({}): Non-emulated device detected - entering NormalOperation", DeviceId()); });
			m_OpState = OperatingStates::NormalOperation;
			Status(Devices::DeviceStatus_Normal{});
		}

		switch (m_OpState)
		{
		case OperatingStates::StartUp:
		{
			LogDebug(Channel::Devices, [this]() { return std::format("IAQ ({}): Processing StartUp state - waiting for MainStatus/AuxStatus", DeviceId()); });
			Status(Devices::DeviceStatus_Initializing{});
			break;
		}

		case OperatingStates::NormalOperation:
		{
			LogTrace(Channel::Devices, [this]() { return std::format("IAQ ({}): Processing NormalOperation state", DeviceId()); });
			break;
		}

		case OperatingStates::FaultHasOccurred:
		{
			LogWarning(Channel::Devices, [this]() { return std::format("IAQ ({}): Processing FaultHasOccurred state", DeviceId()); });
			break;
		}
		}

		// Commands can only be sent in response to IAQ_Poll messages.
		if (is_poll_message && !m_CommandQueue.empty())
		{
			auto cmd = m_CommandQueue.front();
			m_CommandQueue.pop_front();
			LogDebug(Channel::Devices, [&]() { return std::format("IAQ ({}): Sending queued command in Poll ACK: 0x{:02x} ({} remaining)",
				DeviceId(), cmd, m_CommandQueue.size()); });
			Signal_AckMessage(static_cast<uint8_t>(0x00), cmd);
		}
		else if (is_poll_message && m_PendingCommand != 0x00)
		{
			LogDebug(Channel::Devices, [this]() { return std::format("IAQ ({}): Sending command in Poll ACK: 0x{:02x}", DeviceId(), m_PendingCommand); });
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

		if (m_OpState == OperatingStates::NotPresent)
		{
			// Already determined this id is not present on the bus -- stay quiet.
			return;
		}

		LogWarning(Channel::Devices, [this]() { return std::format("IAQ ({}): Watchdog timeout occurred: state={}, timeout_duration={}s", DeviceId(), magic_enum::enum_name(m_OpState), IAQ_TIMEOUT_DURATION.count()); });

		if (m_OpState == OperatingStates::StartUp)
		{
			if (m_HasReceivedData)
			{
				// Traffic addressed to this id was being received and then stopped -- a
				// genuine fault (the device was present but went silent).
				LogWarning(Channel::Devices, [this]() { return std::format("IAQ ({}): No valid data received during StartUp -> entering FaultHasOccurred", DeviceId()); });
				m_OpState = OperatingStates::FaultHasOccurred;
				Status(Devices::DeviceStatus_FaultOccurred{});
			}
			else
			{
				// This id was never addressed on the bus: the master is not polling an
				// iAqualink2 here (e.g. an emulated id the panel isn't configured for).
				// That is "not present", not a fault -- settle quietly rather than alarm.
				LogInfo(Channel::Devices, [this]() { return std::format("IAQ ({}): No traffic ever addressed to this id -> marking NotPresent (master is not polling an iAqualink2 here)", DeviceId()); });
				m_OpState = OperatingStates::NotPresent;
				Status(Devices::DeviceStatus_Initializing{});
			}
		}
	}

	void IAQDevice::Signal_ControlDataResponse(const std::string& ascii_data)
	{
		if (!IsEmulated())
		{
			return;
		}

		auto data_msg = std::make_shared<Messages::IAQMessage_ControlDataResponse>(ascii_data);
		if (data_msg)
		{
			LogDebug(Channel::Devices, [this, &ascii_data]() { return std::format("IAQ ({}): Signalling control data response: '{}'", DeviceId(), ascii_data); });
			data_msg->Signal_MessageToSend();
		}
	}

	nlohmann::json IAQDevice::DescribeDiagnostics() const
	{
		nlohmann::json j;

		j["device_type"] = "IAQ";
		j["device_id"] = std::format("0x{:02x}", DeviceId().Id()());
		j["operating_state"] = std::string(magic_enum::enum_name(m_OpState));

		j["screen"] = DescribeScreen();

		j["pending_command"] = std::format("0x{:02x}", m_PendingCommand);
		j["command_queue_depth"] = static_cast<uint32_t>(m_CommandQueue.size());
		j["awaiting_control_ready"] = m_AwaitingControlReady;
		j["control_data_value"] = m_ControlDataValue;
		j["is_running"] = IsRunning();

		return j;
	}

}
// namespace AqualinkAutomate::Devices
