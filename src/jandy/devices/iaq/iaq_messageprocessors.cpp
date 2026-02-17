#include <format>

#include <magic_enum/magic_enum.hpp>

#include "logging/logging.h"
#include "devices/device_status.h"
#include "devices/iaq_device.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Devices
{

	void IAQDevice::Slot_IAQ_Poll(const Messages::IAQMessage_Poll& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::Slot_IAQ_Poll", std::source_location::current(), UnitColours::Red);

		LogTrace(Channel::Devices, std::format("IAQ ({}): Received IAQMessage_Poll", DeviceId()));

		ProcessControllerUpdates(true);

		Restartable::Kick();
	}

	void IAQDevice::Slot_IAQ_MainStatus(const Messages::IAQMessage_MainStatus& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::Slot_IAQ_MainStatus", std::source_location::current(), UnitColours::Red);

		LogTrace(Channel::Devices, std::format("IAQ ({}): Received IAQMessage_MainStatus: {}", DeviceId(), msg.ToString()));

		ProcessMainStatus(msg);

		if (m_OpState == OperatingStates::StartUp)
		{
			LogInfo(Channel::Devices, std::format("IAQ ({}): MainStatus received during StartUp -> entering NormalOperation", DeviceId()));
			m_OpState = OperatingStates::NormalOperation;
			Status(Devices::DeviceStatus_Normal{});
		}

		ProcessControllerUpdates();

		Restartable::Kick();
	}

	void IAQDevice::Slot_IAQ_AuxStatus(const Messages::IAQMessage_AuxStatus& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::Slot_IAQ_AuxStatus", std::source_location::current(), UnitColours::Red);

		LogTrace(Channel::Devices, std::format("IAQ ({}): Received IAQMessage_AuxStatus: {}", DeviceId(), msg.ToString()));

		ProcessAuxStatus(msg);

		if (m_OpState == OperatingStates::StartUp)
		{
			LogInfo(Channel::Devices, std::format("IAQ ({}): AuxStatus received during StartUp -> entering NormalOperation", DeviceId()));
			m_OpState = OperatingStates::NormalOperation;
			Status(Devices::DeviceStatus_Normal{});
		}

		ProcessControllerUpdates();

		Restartable::Kick();
	}

	void IAQDevice::Slot_IAQ_PageStart(const Messages::IAQMessage_PageStart& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::Slot_IAQ_PageStart", std::source_location::current(), UnitColours::Red);

		LogTrace(Channel::Devices, std::format("IAQ ({}): Received IAQMessage_PageStart", DeviceId()));

		m_SM_PageUpdate.process_event(Utility::ScreenDataPageUpdaterImpl::evSequenceStart());
		m_SM_PageUpdate.process_event(Utility::ScreenDataPageUpdaterImpl::evClear());

		ProcessControllerUpdates();

		Restartable::Kick();
	}

	void IAQDevice::Slot_IAQ_PageMessage(const Messages::IAQMessage_PageMessage& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::Slot_IAQ_PageMessage", std::source_location::current(), UnitColours::Red);

		LogTrace(Channel::Devices, std::format("IAQ ({}): Received IAQMessage_PageMessage: line_id={}, content='{}'", DeviceId(), msg.LineId(), msg.Line()));

		if (IAQ_STATUS_PAGE_LINES <= msg.LineId())
		{
			LogWarning(Channel::Devices, std::format("IAQ ({}): PageMessage for unsupported line: line_id={} (max={}), content='{}'", DeviceId(), msg.LineId(), IAQ_STATUS_PAGE_LINES - 1, msg.Line()));
		}
		else
		{
			m_SM_PageUpdate.process_event(Utility::ScreenDataPageUpdaterImpl::evUpdate(msg.LineId(), msg.Line()));
		}

		ProcessControllerUpdates();

		Restartable::Kick();
	}

	void IAQDevice::Slot_IAQ_PageEnd(const Messages::IAQMessage_PageEnd& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::Slot_IAQ_PageEnd", std::source_location::current(), UnitColours::Red);

		LogTrace(Channel::Devices, std::format("IAQ ({}): Received IAQMessage_PageEnd", DeviceId()));

		m_SM_PageUpdate.process_event(Utility::ScreenDataPageUpdaterImpl::evSequenceEnd());

		ProcessControllerUpdates();

		Restartable::Kick();
	}

	void IAQDevice::Slot_IAQ_PageContinue(const Messages::IAQMessage_PageContinue& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::Slot_IAQ_PageContinue", std::source_location::current(), UnitColours::Red);

		LogTrace(Channel::Devices, std::format("IAQ ({}): Received IAQMessage_PageContinue", DeviceId()));

		ProcessControllerUpdates();

		Restartable::Kick();
	}

	void IAQDevice::Slot_IAQ_PageButton(const Messages::IAQMessage_PageButton& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::Slot_IAQ_PageButton", std::source_location::current(), UnitColours::Red);

		LogDebug(Channel::Devices, std::format("IAQ ({}): Received IAQMessage_PageButton: index={}, name='{}', type={}, status={}",
			DeviceId(), msg.ButtonIndex(), msg.ButtonName(), magic_enum::enum_name(msg.ButtonType()), magic_enum::enum_name(msg.ButtonStatus())));

		ProcessControllerUpdates();

		Restartable::Kick();
	}

	void IAQDevice::Slot_IAQ_TitleMessage(const Messages::IAQMessage_TitleMessage& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::Slot_IAQ_TitleMessage", std::source_location::current(), UnitColours::Red);

		LogDebug(Channel::Devices, std::format("IAQ ({}): Received IAQMessage_TitleMessage: title='{}'", DeviceId(), msg.Title()));

		ProcessControllerUpdates();

		Restartable::Kick();
	}

	void IAQDevice::Slot_IAQ_TableMessage(const Messages::IAQMessage_TableMessage& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::Slot_IAQ_TableMessage", std::source_location::current(), UnitColours::Red);

		LogTrace(Channel::Devices, std::format("IAQ ({}): Received IAQMessage_TableMessage: line_id={}, content='{}'", DeviceId(), msg.LineId(), msg.Line()));

		if (IAQ_MESSAGE_TABLE_LINES <= msg.LineId())
		{
			LogWarning(Channel::Devices, std::format("IAQ ({}): TableMessage for unsupported line: line_id={} (max={}), content='{}'", DeviceId(), msg.LineId(), IAQ_MESSAGE_TABLE_LINES - 1, msg.Line()));
		}
		else
		{
			m_TableInfo[msg.LineId()].Text = msg.Line();
		}

		ProcessControllerUpdates();

		Restartable::Kick();
	}

	void IAQDevice::Slot_IAQ_MessageLong(const Messages::IAQMessage_MessageLong& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::Slot_IAQ_MessageLong", std::source_location::current(), UnitColours::Red);

		LogTrace(Channel::Devices, std::format("IAQ ({}): Received IAQMessage_MessageLong", DeviceId()));

		ProcessControllerUpdates();

		Restartable::Kick();
	}

	void IAQDevice::Slot_IAQ_StartUp(const Messages::IAQMessage_StartUp& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::Slot_IAQ_StartUp", std::source_location::current(), UnitColours::Red);

		LogInfo(Channel::Devices, std::format("IAQ ({}): Received IAQMessage_StartUp - controller startup notification", DeviceId()));

		ProcessControllerUpdates();

		Restartable::Kick();
	}

	void IAQDevice::Slot_IAQ_ControlReady(const Messages::IAQMessage_ControlReady& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::Slot_IAQ_ControlReady", std::source_location::current(), UnitColours::Red);

		LogInfo(Channel::Devices, std::format("IAQ ({}): Received IAQMessage_ControlReady", DeviceId()));

		ProcessControllerUpdates();

		Restartable::Kick();
	}

	void IAQDevice::Slot_IAQ_CommandReady(const Messages::IAQMessage_CommandReady& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::Slot_IAQ_CommandReady", std::source_location::current(), UnitColours::Red);

		LogInfo(Channel::Devices, std::format("IAQ ({}): Received IAQMessage_CommandReady", DeviceId()));

		ProcessControllerUpdates();

		Restartable::Kick();
	}

	void IAQDevice::Slot_IAQ_OneTouchStatus(const Messages::IAQMessage_OneTouchStatus& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::Slot_IAQ_OneTouchStatus", std::source_location::current(), UnitColours::Red);

		LogTrace(Channel::Devices, std::format("IAQ ({}): Received IAQMessage_OneTouchStatus", DeviceId()));

		ProcessControllerUpdates();

		Restartable::Kick();
	}

	void IAQDevice::Slot_IAQ_Heartbeat(const Messages::IAQMessage_Heartbeat& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::Slot_IAQ_Heartbeat", std::source_location::current(), UnitColours::Red);

		LogTrace(Channel::Devices, std::format("IAQ ({}): Received IAQMessage_Heartbeat", DeviceId()));

		// The IAQ device responds to Heartbeat (0x53) with a constant
		// presence beacon ACK: Type=0x1f, Command=0x00.  This differs from
		// IAQ_Poll (0x30) which can carry commands.
		Signal_AckMessage(static_cast<uint8_t>(0x1f), static_cast<uint8_t>(0x00));

		Restartable::Kick();
	}

}
// namespace AqualinkAutomate::Devices
