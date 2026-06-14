#include <format>

#include <magic_enum/magic_enum.hpp>

#include "logging/logging.h"
#include "devices/device_status.h"
#include "devices/iaq_device.h"
#include "utility/spa_switch_assignment.h"
#include "utility/string_manipulation.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Profiling;

namespace AqualinkAutomate::Devices
{

	void IAQDevice::Slot_IAQ_Poll(const Messages::IAQMessage_Poll& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::Slot_IAQ_Poll", std::source_location::current(), UnitColours::Red);

		LogTrace(Channel::Devices, [this]() { return std::format("IAQ ({}): Received IAQMessage_Poll", DeviceId()); });

		ProcessControllerUpdates(true);

		Restartable::Kick();
	}

	void IAQDevice::Slot_IAQ_Probe(const Messages::JandyMessage_Probe& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::Slot_IAQ_Probe", std::source_location::current(), UnitColours::Red);

		// The master discovers an AqualinkTouch (0x30-0x33) with a generic Probe (0x00),
		// exactly as it discovers a OneTouch -- so an EMULATED instance must answer the
		// probe for the master to see it and begin the iAQ status/page protocol. A passive
		// (non-emulated) decoder must NOT treat a bare probe as a real device answering
		// (that would mask not-present detection), so this only acts when emulated.
		if (!IsEmulated())
		{
			return;
		}

		LogTrace(Channel::Devices, [this]() { return std::format("IAQ ({}): Received JandyMessage_Probe (emulated -> answering)", DeviceId()); });

		ProcessControllerUpdates();

		Restartable::Kick();
	}

	void IAQDevice::Slot_IAQ_MainStatus(const Messages::IAQMessage_MainStatus& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::Slot_IAQ_MainStatus", std::source_location::current(), UnitColours::Red);

		LogTrace(Channel::Devices, [&]() { return std::format("IAQ ({}): Received IAQMessage_MainStatus: {}", DeviceId(), msg.ToString()); });

		ProcessMainStatus(msg);

		if (m_OpState == OperatingStates::StartUp)
		{
			LogInfo(Channel::Devices, [this]() { return std::format("IAQ ({}): MainStatus received during StartUp -> entering NormalOperation", DeviceId()); });
			m_OpState = OperatingStates::NormalOperation;
			Status(Devices::DeviceStatus_Normal{});
		}

		ProcessControllerUpdates();

		Restartable::Kick();
	}

	void IAQDevice::Slot_IAQ_AuxStatus(const Messages::IAQMessage_AuxStatus& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::Slot_IAQ_AuxStatus", std::source_location::current(), UnitColours::Red);

		LogTrace(Channel::Devices, [&]() { return std::format("IAQ ({}): Received IAQMessage_AuxStatus: {}", DeviceId(), msg.ToString()); });

		ProcessAuxStatus(msg);

		if (m_OpState == OperatingStates::StartUp)
		{
			LogInfo(Channel::Devices, [this]() { return std::format("IAQ ({}): AuxStatus received during StartUp -> entering NormalOperation", DeviceId()); });
			m_OpState = OperatingStates::NormalOperation;
			Status(Devices::DeviceStatus_Normal{});
		}

		ProcessControllerUpdates();

		Restartable::Kick();
	}

	void IAQDevice::Slot_IAQ_PageStart(const Messages::IAQMessage_PageStart& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::Slot_IAQ_PageStart", std::source_location::current(), UnitColours::Red);

		LogTrace(Channel::Devices, [this]() { return std::format("IAQ ({}): Received IAQMessage_PageStart", DeviceId()); });

		m_SM_PageUpdate.process_event(Utility::ScreenDataPageUpdaterImpl::evSequenceStart());
		m_SM_PageUpdate.process_event(Utility::ScreenDataPageUpdaterImpl::evClear());

		ProcessControllerUpdates();

		Restartable::Kick();
	}

	void IAQDevice::Slot_IAQ_PageMessage(const Messages::IAQMessage_PageMessage& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::Slot_IAQ_PageMessage", std::source_location::current(), UnitColours::Red);

		LogTrace(Channel::Devices, [&]() { return std::format("IAQ ({}): Received IAQMessage_PageMessage: line_id={}, content='{}'", DeviceId(), msg.LineId(), msg.Line()); });

		if (IAQ_STATUS_PAGE_LINES <= msg.LineId())
		{
			LogWarning(Channel::Devices, [&]() { return std::format("IAQ ({}): PageMessage for unsupported line: line_id={} (max={}), content='{}'", DeviceId(), msg.LineId(), IAQ_STATUS_PAGE_LINES - 1, msg.Line()); });
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

		LogTrace(Channel::Devices, [this]() { return std::format("IAQ ({}): Received IAQMessage_PageEnd", DeviceId()); });

		m_SM_PageUpdate.process_event(Utility::ScreenDataPageUpdaterImpl::evSequenceEnd());

		ProcessControllerUpdates();

		Restartable::Kick();
	}

	void IAQDevice::Slot_IAQ_PageContinue(const Messages::IAQMessage_PageContinue& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::Slot_IAQ_PageContinue", std::source_location::current(), UnitColours::Red);

		LogTrace(Channel::Devices, [this]() { return std::format("IAQ ({}): Received IAQMessage_PageContinue", DeviceId()); });

		ProcessControllerUpdates();

		Restartable::Kick();
	}

	void IAQDevice::Slot_IAQ_PageButton(const Messages::IAQMessage_PageButton& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::Slot_IAQ_PageButton", std::source_location::current(), UnitColours::Red);

		LogDebug(Channel::Devices, [&]() { return std::format("IAQ ({}): Received IAQMessage_PageButton: index={}, name='{}', type={}, status={}",
			DeviceId(), msg.ButtonIndex(), msg.ButtonName(), magic_enum::enum_name(msg.ButtonType()), magic_enum::enum_name(msg.ButtonStatus())); });

		// Retain the live button table (index -> name + status) for the CURRENT page so
		// DeviceActuator can resolve a logical aux to its on-screen button index by name.
		// A named button refreshes its entry; an empty/blank name clears that slot so a
		// stale name can't be matched after the page changes.
		if (Utility::TrimWhitespace(msg.ButtonName()).empty())
		{
			m_PageButtons.erase(msg.ButtonIndex());
		}
		else
		{
			m_PageButtons[msg.ButtonIndex()] = PageButtonInfo{ msg.ButtonName(), msg.ButtonStatus() };
		}

		ProcessControllerUpdates();

		Restartable::Kick();
	}

	void IAQDevice::Slot_IAQ_TitleMessage(const Messages::IAQMessage_TitleMessage& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::Slot_IAQ_TitleMessage", std::source_location::current(), UnitColours::Red);

		LogDebug(Channel::Devices, [&]() { return std::format("IAQ ({}): Received IAQMessage_TitleMessage: title='{}'", DeviceId(), msg.Title()); });

		ProcessControllerUpdates();

		Restartable::Kick();
	}

	void IAQDevice::Slot_IAQ_TableMessage(const Messages::IAQMessage_TableMessage& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::Slot_IAQ_TableMessage", std::source_location::current(), UnitColours::Red);

		LogTrace(Channel::Devices, [&]() { return std::format("IAQ ({}): Received IAQMessage_TableMessage: line_id={}, content='{}'", DeviceId(), msg.LineId(), msg.Line()); });

		if (IAQ_MESSAGE_TABLE_LINES <= msg.LineId())
		{
			LogWarning(Channel::Devices, [&]() { return std::format("IAQ ({}): TableMessage for unsupported line: line_id={} (max={}), content='{}'", DeviceId(), msg.LineId(), IAQ_MESSAGE_TABLE_LINES - 1, msg.Line()); });
		}
		else
		{
			m_TableInfo[msg.LineId()].Text = msg.Line();
		}

		// Spa-side switch button assignments appear on the iAQ "Spa Remotes" config page as
		// group-0x00 table rows "<switch>:<button>\t<function>" (the function picker is group
		// 0x01). Decode them into the controller-agnostic DataHub map. The parser only accepts the
		// "N:M <function>" shape, so non-assignment group-0 rows are safely ignored.
		if ((nullptr != m_DataHub) && (0x00 == msg.LineId()))
		{
			if (const auto assignment = Utility::ParseSpaSwitchAssignmentLine(msg.Line()))
			{
				m_DataHub->SetSpaSwitchAssignment(assignment->switch_number, assignment->button_number, assignment->function);
				LogDebug(Channel::Devices, [&]() { return std::format("IAQ ({}): spa-switch assignment {}:{} -> '{}'", DeviceId(), assignment->switch_number, assignment->button_number, assignment->function); });
			}
		}

		ProcessControllerUpdates();

		Restartable::Kick();
	}

	void IAQDevice::Slot_IAQ_MessageLong(const Messages::IAQMessage_MessageLong& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::Slot_IAQ_MessageLong", std::source_location::current(), UnitColours::Red);

		LogTrace(Channel::Devices, [this]() { return std::format("IAQ ({}): Received IAQMessage_MessageLong", DeviceId()); });

		ProcessControllerUpdates();

		Restartable::Kick();
	}

	void IAQDevice::Slot_IAQ_StartUp(const Messages::IAQMessage_StartUp& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::Slot_IAQ_StartUp", std::source_location::current(), UnitColours::Red);

		LogInfo(Channel::Devices, [this]() { return std::format("IAQ ({}): Received IAQMessage_StartUp - controller startup notification", DeviceId()); });

		ProcessControllerUpdates();

		Restartable::Kick();
	}

	void IAQDevice::Slot_IAQ_ControlReady(const Messages::IAQMessage_ControlReady& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::Slot_IAQ_ControlReady", std::source_location::current(), UnitColours::Red);

		LogInfo(Channel::Devices, [this]() { return std::format("IAQ ({}): Received IAQMessage_ControlReady", DeviceId()); });

		if (m_AwaitingControlReady && !m_ControlDataValue.empty())
		{
			LogInfo(Channel::Devices, [this]() { return std::format("IAQ ({}): Sending control data response: '{}'", DeviceId(), m_ControlDataValue); });
			Signal_ControlDataResponse(m_ControlDataValue);
			m_AwaitingControlReady = false;
			m_ControlDataValue.clear();
		}
		else
		{
			ProcessControllerUpdates();
		}

		Restartable::Kick();
	}

	void IAQDevice::Slot_IAQ_CommandReady(const Messages::IAQMessage_CommandReady& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::Slot_IAQ_CommandReady", std::source_location::current(), UnitColours::Red);

		LogInfo(Channel::Devices, [this]() { return std::format("IAQ ({}): Received IAQMessage_CommandReady", DeviceId()); });

		ProcessControllerUpdates();

		Restartable::Kick();
	}

	void IAQDevice::Slot_IAQ_OneTouchStatus(const Messages::IAQMessage_OneTouchStatus& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::Slot_IAQ_OneTouchStatus", std::source_location::current(), UnitColours::Red);

		LogTrace(Channel::Devices, [this]() { return std::format("IAQ ({}): Received IAQMessage_OneTouchStatus", DeviceId()); });

		ProcessControllerUpdates();

		Restartable::Kick();
	}

	void IAQDevice::Slot_IAQ_Heartbeat(const Messages::IAQMessage_Heartbeat& msg)
	{
		auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("IAQDevice::Slot_IAQ_Heartbeat", std::source_location::current(), UnitColours::Red);

		LogTrace(Channel::Devices, [this]() { return std::format("IAQ ({}): Received IAQMessage_Heartbeat", DeviceId()); });

		// The IAQ device responds to Heartbeat (0x53) with a constant
		// presence beacon ACK: Type=0x1f, Command=0x00.  This differs from
		// IAQ_Poll (0x30) which can carry commands.
		Signal_AckMessage(static_cast<uint8_t>(0x1f), static_cast<uint8_t>(0x00));

		// A heartbeat proves a real (non-emulated) iAqualink2 is alive and present.
		// Its rich status (MainStatus/AuxStatus) is carried on the AqualinkTouch 0x33
		// side, not here on 0xA0-0xA3, so the heartbeat is the only start-up signal a
		// snooped 0xa3 device ever sees. Treat it as valid activity so the device
		// reaches NormalOperation instead of sitting in StartUp until the watchdog
		// eventually faults it (which is exactly the "IAQ didn't start up properly"
		// symptom on a system with a real iAqualink2).
		if (!IsEmulated() && (m_OpState == OperatingStates::StartUp))
		{
			LogInfo(Channel::Devices, [this]() { return std::format("IAQ ({}): Heartbeat received during StartUp -> entering NormalOperation", DeviceId()); });
			m_OpState = OperatingStates::NormalOperation;
			Status(Devices::DeviceStatus_Normal{});
		}

		Restartable::Kick();

		// Distinguish the two IAQ ids that share this handler: the AqualinkTouch 0x33
		// side carries MainStatus and renders a System Status page (and must KEEP it
		// even if a heartbeat arrives), whereas the heartbeat-only 0xA3 cloud interface
		// never sees a MainStatus -- render its Cloud Link page from the heartbeat so it
		// shows live link liveness instead of the constructor-default Page_Unknown.
		if (!m_HasReceivedMainStatus)
		{
			RenderCloudLinkScreen();
		}
	}

}
// namespace AqualinkAutomate::Devices
