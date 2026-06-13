#include <format>
#include <functional>

#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>

#include "auxillaries/jandy_auxillary_traits_types.h"
#include "devices/device_status.h"
#include "logging/logging.h"
#include "devices/serial_adapter_device.h"
#include "kernel/auxillary_devices/auxillary_device.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "utility/overloaded_variant_visitor.h"

using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Messages;
using namespace AqualinkAutomate::Kernel::AuxillaryTraitsTypes;

namespace AqualinkAutomate::Devices
{

	SerialAdapterDevice::SerialAdapterDevice(const std::shared_ptr<Devices::JandyDeviceType>& device_id, Kernel::HubLocator& hub_locator, bool is_emulated) :
		JandyController(device_id, hub_locator),
		Capabilities::Restartable(SERIALADAPTER_TIMEOUT_DURATION),
		Capabilities::Emulated(is_emulated),
		m_StatusTypesCollection(),
		m_StatusTypesCollectionIter(),
		m_StatusMessageReceived(false),
		m_PendingCommands(),
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
			if (!m_PendingCommands.empty())
			{
				auto zone = Factory::ProfilingUnitFactory::Instance().CreateZone("SerialAdapterDevice::ProcessControllerUpdates -> pending_command", std::source_location::current());

				const PendingCommand pending = m_PendingCommands.front();
				m_PendingCommands.pop_front();

				LogDebug(Channel::Devices, std::format("ProcessControllerUpdates -> Sending pending command (ack_type=0x{:02x}, ack_data_value=0x{:02x}); {} remaining", pending.ack_type, pending.ack_data_value, m_PendingCommands.size()));

				ack_type = pending.ack_type;
				ack_data_value = pending.ack_data_value;
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
		if (IsEmulated() && IsEmulationSuppressed())
		{
			// Presence gating: a real adapter owns this address. Never queue work
			// for a suppressed instance -- it must not transmit.
			LogWarning(Channel::Devices, std::format("SerialAdapterDevice: Ignoring command (ack_type=0x{:02x}, ack_data_value=0x{:02x}) -- emulation is suppressed (a real adapter is present).", ack_type, ack_data_value));
			return;
		}

		// Single-command semantics: a freshly issued command supersedes any earlier
		// still-pending command (last-write-wins), so we clear before queuing. The
		// multi-step write path uses EnqueueCommand() to append without clobbering.
		LogDebug(Channel::Devices, std::format("SerialAdapterDevice: Queuing command (ack_type=0x{:02x}, ack_data_value=0x{:02x})", ack_type, ack_data_value));
		m_PendingCommands.clear();
		m_PendingCommands.emplace_back(PendingCommand{ ack_type, ack_data_value });
	}

	void SerialAdapterDevice::EnqueueCommand(uint8_t ack_type, uint8_t ack_data_value)
	{
		if (IsEmulated() && IsEmulationSuppressed())
		{
			// Presence gating: a real adapter owns this address; stay passive.
			LogWarning(Channel::Devices, std::format("SerialAdapterDevice: Ignoring enqueued command (ack_type=0x{:02x}, ack_data_value=0x{:02x}) -- emulation is suppressed (a real adapter is present).", ack_type, ack_data_value));
			return;
		}

		// Append (FIFO) -- used by multi-step writes that must emit several ACKs in
		// a specific order, one per master poll, without discarding earlier steps.
		LogDebug(Channel::Devices, std::format("SerialAdapterDevice: Enqueuing command (ack_type=0x{:02x}, ack_data_value=0x{:02x})", ack_type, ack_data_value));
		m_PendingCommands.emplace_back(PendingCommand{ ack_type, ack_data_value });
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

	Capabilities::ActuationResult SerialAdapterDevice::ActuateDevice(const std::shared_ptr<Kernel::AuxillaryDevice>& device, Capabilities::ActuationAction requested_action)
	{
		// Determine the serial command based on the requested action.
		auto action = SerialAdapter_CommandTypes::SetOn;

		if (requested_action == Capabilities::ActuationAction::Off)
		{
			action = SerialAdapter_CommandTypes::SetOff;
		}
		else if (requested_action == Capabilities::ActuationAction::Toggle)
		{
			// Auto-detect: default to SetOn; if device is already on, use SetOff.
			if (device->AuxillaryTraits.Has(AuxillaryTypeTrait{}))
			{
				auto device_type = *(device->AuxillaryTraits[AuxillaryTypeTrait{}]);

				switch (device_type)
				{
				case AuxillaryTypes::Auxillary:
				case AuxillaryTypes::Cleaner:
				case AuxillaryTypes::Spillover:
				case AuxillaryTypes::Sprinkler:
					if (auto status = device->AuxillaryTraits.TryGet(AuxillaryStatusTrait{}); status.has_value() && *status == Kernel::AuxillaryStatuses::On)
					{
						action = SerialAdapter_CommandTypes::SetOff;
					}
					break;

				case AuxillaryTypes::Pump:
					if (auto status = device->AuxillaryTraits.TryGet(PumpStatusTrait{}); status.has_value() && *status == Kernel::PumpStatuses::Running)
					{
						action = SerialAdapter_CommandTypes::SetOff;
					}
					break;

				default:
					break;
				}
			}
		}

		// Check if the device has a JandyAuxillaryId trait (i.e. a hardware aux port).
		if (device->AuxillaryTraits.Has(Auxillaries::JandyAuxillaryId{}))
		{
			auto aux_id = *(device->AuxillaryTraits[Auxillaries::JandyAuxillaryId{}]);
			LogInfo(Channel::Devices, std::format("SerialAdapterDevice: Actuating aux command for {} (action=0x{:02x})", magic_enum::enum_name(aux_id), magic_enum::enum_integer(action)));
			QueueAuxCommand(aux_id, action);
			return Capabilities::ActuationResult::Accepted;
		}

		// No aux ID - try to map as a system pump command via type and label.
		if (device->AuxillaryTraits.Has(AuxillaryTypeTrait{}))
		{
			auto device_type = *(device->AuxillaryTraits[AuxillaryTypeTrait{}]);
			auto label_opt = device->AuxillaryTraits.TryGet(LabelTrait{});
			std::string label = label_opt.has_value() ? *label_opt : "";

			switch (device_type)
			{
			case AuxillaryTypes::Pump:
				if (label.find("Filter") != std::string::npos || label.find("Pump") != std::string::npos)
				{
					LogInfo(Channel::Devices, std::format("SerialAdapterDevice: Actuating pump command PUMP (action=0x{:02x})", magic_enum::enum_integer(action)));
					QueuePumpCommand(SerialAdapter_SystemPumpCommands::PUMP, action);
					return Capabilities::ActuationResult::Accepted;
				}
				break;

			case AuxillaryTypes::Cleaner:
				LogInfo(Channel::Devices, std::format("SerialAdapterDevice: Actuating pump command CLEANR (action=0x{:02x})", magic_enum::enum_integer(action)));
				QueuePumpCommand(SerialAdapter_SystemPumpCommands::CLEANR, action);
				return Capabilities::ActuationResult::Accepted;

			case AuxillaryTypes::Spillover:
				LogInfo(Channel::Devices, std::format("SerialAdapterDevice: Actuating pump command SPILLOVER (action=0x{:02x})", magic_enum::enum_integer(action)));
				QueuePumpCommand(SerialAdapter_SystemPumpCommands::SPILLOVER, action);
				return Capabilities::ActuationResult::Accepted;

			default:
				break;
			}

			// Check if it's a spa device by label.
			if (label.find("Spa") != std::string::npos)
			{
				LogInfo(Channel::Devices, std::format("SerialAdapterDevice: Actuating pump command SPA (action=0x{:02x})", magic_enum::enum_integer(action)));
				QueuePumpCommand(SerialAdapter_SystemPumpCommands::SPA, action);
				return Capabilities::ActuationResult::Accepted;
			}
		}

		LogWarning(Channel::Devices, "SerialAdapterDevice: Could not map device to a serial adapter command");
		return Capabilities::ActuationResult::MappingFailed;
	}

	Capabilities::ActuationResult SerialAdapterDevice::SetPoolSetpoint(uint8_t temperature)
	{
		LogInfo(Channel::Devices, std::format("SerialAdapterDevice: Setting pool setpoint to {}", temperature));
		QueueSetpointCommand(SerialAdapter_SystemTemperatureCommands::POOLSP, temperature);
		return Capabilities::ActuationResult::Accepted;
	}

	Capabilities::ActuationResult SerialAdapterDevice::SetSpaSetpoint(uint8_t temperature)
	{
		LogInfo(Channel::Devices, std::format("SerialAdapterDevice: Setting spa setpoint to {}", temperature));
		QueueSetpointCommand(SerialAdapter_SystemTemperatureCommands::SPASP, temperature);
		return Capabilities::ActuationResult::Accepted;
	}

	Capabilities::ActuationResult SerialAdapterDevice::SetCirculationMode(Kernel::CirculationModes mode)
	{
		switch (mode)
		{
		case Kernel::CirculationModes::Spa:
			LogInfo(Channel::Devices, "SerialAdapterDevice: Setting circulation mode to Spa");
			QueuePumpCommand(SerialAdapter_SystemPumpCommands::SPA, SerialAdapter_CommandTypes::SetOn);
			return Capabilities::ActuationResult::Accepted;

		case Kernel::CirculationModes::Pool:
			LogInfo(Channel::Devices, "SerialAdapterDevice: Setting circulation mode to Pool");
			QueuePumpCommand(SerialAdapter_SystemPumpCommands::SPA, SerialAdapter_CommandTypes::SetOff);
			return Capabilities::ActuationResult::Accepted;

		case Kernel::CirculationModes::Spillover:
			LogInfo(Channel::Devices, "SerialAdapterDevice: Setting circulation mode to Spillover");
			QueuePumpCommand(SerialAdapter_SystemPumpCommands::SPILLOVER, SerialAdapter_CommandTypes::SetOn);
			return Capabilities::ActuationResult::Accepted;

		case Kernel::CirculationModes::SpaFill:
		case Kernel::CirculationModes::SpaDrain:
			// Spa Fill / Spa Drain are valve-sequencing / service operations: the Jandy RS
			// protocol has no single command for them (the RS Serial Adapter exposes only the
			// SPA and SPILLOVER pump commands), so they cannot be driven remotely. Report
			// NotSupported (a well-formed request this controller cannot perform) rather than
			// the generic InvalidValue. See OBS-06 in docs/alwin32/sim-validation-findings.md.
			LogWarning(Channel::Devices, std::format("SerialAdapterDevice: Circulation mode {} is not remotely commandable - no RS-485 command exists for it", magic_enum::enum_name(mode)));
			return Capabilities::ActuationResult::NotSupported;

		default:
			LogWarning(Channel::Devices, std::format("SerialAdapterDevice: Invalid circulation mode: {}", magic_enum::enum_name(mode)));
			return Capabilities::ActuationResult::InvalidValue;
		}
	}

	void SerialAdapterDevice::QueueSetpointWrite_TwoStep(Messages::SerialAdapter_SystemTemperatureCommands setpoint, uint8_t temperature)
	{
		// CAPTURE-GATED WRITE (AqualinkD source/serialadapter.c, two-step setpoint).
		// Byte ordering/codes are AqualinkD-derived and NOT yet validated on this
		// project's live bus -- proven only by synthetic round-trip tests here.
		// MUST be confirmed against a live Brainboxes capture before trusting it to
		// drive real hardware.
		//
		//   Step 1 (readySP): wire {0x00,0x01,typeID,0x35}
		//                     -> ACK {ack_type = typeID(setpoint), data = 0x35}
		//   Step 2 (setSP):   wire {0x00,0x01,0x00,val}
		//                     -> ACK {ack_type = 0x00,            data = temperature}
		//
		// The two ACKs are queued FIFO so the readySP is emitted on the next master
		// poll and the setSP on the poll immediately after, mirroring the adapter's
		// expected request/confirm handshake order.
		LogNotify(Channel::Devices, std::format("SerialAdapterDevice: [CAPTURE-GATED] Queuing two-step setpoint write ({}, temperature={}) -- readySP then setSP", magic_enum::enum_name(setpoint), temperature));

		// Supersede any earlier pending command, then append both steps in order so
		// readySP and setSP go out on consecutive polls. The second QueueCommand
		// must NOT be used (it would clear the first step) -- EnqueueCommand appends.
		QueueCommand(magic_enum::enum_integer(setpoint), magic_enum::enum_integer(Messages::SerialAdapter_CommandTypes::ReadySetpoint));
		EnqueueCommand(0x00, temperature);
	}

	void SerialAdapterDevice::QueueAuxToggleWrite(Auxillaries::JandyAuxillaryIds aux_id, bool turn_on)
	{
		// CAPTURE-GATED WRITE (AqualinkD source/serialadapter.c, aux on/off toggle).
		//   setDev[] = {0x00, 0x01, state, devID}, state = RS_SA_ON(0x81)/OFF(0x80).
		//   -> ACK {ack_type = state, data = devID(aux + SERIALADAPTER_AUX_ID_OFFSET)}
		//
		// NOTE the byte ORDER differs from the existing QueueAuxCommand() (which
		// sends {ack_type = devID, data = state}). AqualinkD's setDev puts the state
		// in the command byte and the device id in the value byte. This ordering is
		// AqualinkD-derived and NOT yet validated on this project's live bus -- it is
		// proven only by synthetic round-trip tests here and MUST be reconciled
		// against a live Brainboxes capture before it is used to drive hardware.
		const uint8_t state{ static_cast<uint8_t>(turn_on ? Messages::SerialAdapter_CommandTypes::SetOn : Messages::SerialAdapter_CommandTypes::SetOff) };
		const uint8_t dev_id{ static_cast<uint8_t>(magic_enum::enum_integer(aux_id) + Messages::SerialAdapterMessage_DevStatus::SERIALADAPTER_AUX_ID_OFFSET) };

		LogNotify(Channel::Devices, std::format("SerialAdapterDevice: [CAPTURE-GATED] Queuing aux toggle write ({}, {}) -> setDev state=0x{:02x} devID=0x{:02x}", magic_enum::enum_name(aux_id), turn_on ? "ON" : "OFF", state, dev_id));

		QueueCommand(state, dev_id);
	}

	void SerialAdapterDevice::WatchdogTimeoutOccurred()
	{
	}

	nlohmann::json SerialAdapterDevice::DescribeDiagnostics() const
	{
		nlohmann::json j;

		j["device_type"] = "SerialAdapter";
		j["device_id"] = std::format("0x{:02x}", DeviceId().Id()());
		j["status_collection_count"] = static_cast<uint32_t>(m_StatusTypesCollection.size());
		j["status_message_received"] = m_StatusMessageReceived;
		j["has_pending_command"] = !m_PendingCommands.empty();
		j["pending_command_count"] = static_cast<uint32_t>(m_PendingCommands.size());
		j["is_emulated"] = IsEmulated();
		j["emulation_suppressed"] = IsEmulationSuppressed();
		j["is_running"] = IsRunning();

		return j;
	}

}
// namespace AqualinkAutomate::Devices
