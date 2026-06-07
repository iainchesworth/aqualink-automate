#pragma once

#include <cstdint>

#include <magic_enum/magic_enum.hpp>

namespace AqualinkAutomate::Messages
{

	enum class JandyMessageIds : uint8_t
	{
		Probe = 0x00,
		Ack = 0x01,
		Status = 0x02,
		Message = 0x03,
		MessageLong = 0x04,

		// Heater commands
		Heater_Request = 0x0C,
		Heater_Status = 0x0D,

		// Serial Adapter commands
		RSSA_DevStatus = 0x13,
		RSSA_DevReady = 0x07,

		// AQUARITE commands
		AQUARITE_Percent = 0x11,
		AQUARITE_GetId = 0x14,
		AQUARITE_PPM = 0x16,

		// PDA commands
		PDA_Highlight = 0x08,
		PDA_Clear = 0x09,
		PDA_ShiftLines = 0x0F,
		PDA_HighlightChars = 0x10,

		// ePump commands
		EPUMP_Status = 0x1F,
		EPUMP_RPM = 0x44,
		EPUMP_Watts = 0x45,

		// Jandy Light commands
		// Jandy colored-light controllers occupy device ids 0xF0-0xF4 on the
		// RS-485 bus.  The light's status reply carries its on/off state and
		// the active colour/show mode.  AqualinkD documents only the device-id
		// range for these controllers (their wire payload is not otherwise
		// reverse-engineered), so this status command byte models the reply so
		// the decode can be extended once a live capture is available.
		Light_Status = 0x19,

		// Chemlink commands
		Chemlink_Response = 0x21,

		// IAQ commands
		IAQ_PageStart = 0x23,
		IAQ_PageButton = 0x24,
		IAQ_PageMessage = 0x25,
		IAQ_TableMessage = 0x26,
		IAQ_PageEnd = 0x28,
		IAQ_StartUp = 0x29,
		IAQ_MessageLong = 0x2C,
		IAQ_TitleMessage = 0x2D,
		IAQ_Poll = 0x30,
		IAQ_ControlReady = 0x31,
		IAQ_PageContinue = 0x40,
		IAQ_MainStatus = 0x70,
		IAQ_OneTouchStatus = 0x71,
		IAQ_AuxStatus = 0x72,
		IAQ_Heartbeat = 0x53,
		IAQ_CommandReady = 0x73,

		// Display protocol
		DisplayUpdate = 0x05,

		// Unknown ids
		Unknown_PDA_1B = 0x1B,
		Unknown_ReadyControl = 0x80,
		Unknown = 0xFF
	};

}
// namespace AqualinkAutomate::Messages

template <>
struct magic_enum::customize::enum_range<AqualinkAutomate::Messages::JandyMessageIds>
{
	static constexpr int min = 0x00;
	static constexpr int max = 0xFF;
	// (max - min) must be less than UINT16_MAX.
};
