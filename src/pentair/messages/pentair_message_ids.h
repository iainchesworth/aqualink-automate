#pragma once

#include <cstdint>

#include <magic_enum/magic_enum.hpp>

namespace AqualinkAutomate::Pentair::Messages
{

	// Pentair RS-485 command byte (the CMD field of the frame header).
	//
	// Only the commands the application currently decodes/emits are enumerated;
	// any unrecognised CMD byte deserialises into a PentairMessage_Unknown so
	// unknown traffic is still framed, counted, and logged.
	enum class PentairMessageIds : uint8_t
	{
		Unknown = 0x00,

		// Variable-speed pump (IntelliFlo) commands.
		Pump_Speed   = 0x01, // Set pump speed (RPM).
		Pump_Remote  = 0x04, // Take/release remote (local) control.
		Pump_Power   = 0x06, // Pump power on/off.
		Pump_Status  = 0x07, // Pump status broadcast (RPM / watts / GPM).
	};

}
// namespace AqualinkAutomate::Pentair::Messages
