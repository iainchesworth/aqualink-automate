#pragma once

#include <cstdint>

namespace AqualinkAutomate::Messages
{

	enum class JandyMessageIds : uint8_t
	{
		Probe = 0x00,
		Ack = 0x01,
		Status = 0x02,
		Message = 0x03,
		MessageLong = 0x04,
		MessageLoopStart = 0x08,

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

		// IAQ commands
		IAQ_PageMessage = 0x25,
		IAQ_TableMessage = 0x26,
		IAQ_PageButton = 0x24,
		IAQ_PageStart = 0x23,
		IAQ_PageEnd = 0x28,
		IAQ_StartUp = 0x29,
		IAQ_Poll = 0x30,
		IAQ_ControlReady = 0x31,
		IAQ_PageContinue = 0x40,
		IAQ_MessageLong = 0x2C,

		// Unknown ids
		Unknown_05 = 0x05,
		Unknown_PDA_1B = 0x1B,
		Unknown_IAQ_2D = 0x2D,
		Unknown_IAQ_70 = 0x70,
		Unknown_ReadyControl = 0x80,
		Unknown = 0xFF
	};

}
// namespace AqualinkAutomate::Messages
