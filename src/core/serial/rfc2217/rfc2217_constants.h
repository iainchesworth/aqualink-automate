#pragma once

#include <cstdint>

namespace AqualinkAutomate::Serial::RFC2217
{

	namespace Constants
	{

		constexpr uint8_t IAC = 0xFF;  // Interpret As Command
		constexpr uint8_t SB = 0xFA;   // Subnegotiation Begin
		constexpr uint8_t SE = 0xF0;   // Subnegotiation End
		constexpr uint8_t WILL = 0xFB;
		constexpr uint8_t WONT = 0xFC;
		constexpr uint8_t DO = 0xFD;
		constexpr uint8_t DONT = 0xFE;

		constexpr uint8_t COM_PORT_OPTION = 44;

		// COM Port Control Commands
		constexpr uint8_t SET_BAUDRATE = 1;
		constexpr uint8_t SET_DATASIZE = 2;
		constexpr uint8_t SET_PARITY = 3;
		constexpr uint8_t SET_STOPSIZE = 4;
		constexpr uint8_t SET_CONTROL = 5;
		constexpr uint8_t NOTIFY_LINESTATE = 6;
		constexpr uint8_t NOTIFY_MODEMSTATE = 7;
		constexpr uint8_t FLOWCONTROL_SUSPEND = 8;
		constexpr uint8_t FLOWCONTROL_RESUME = 9;
		constexpr uint8_t SET_LINESTATE_MASK = 10;
		constexpr uint8_t SET_MODEMSTATE_MASK = 11;
		constexpr uint8_t PURGE_DATA = 12;

		// Server to Client offset
		constexpr uint8_t SERVER_OFFSET = 100;

	}
	// namespace Constants

}
// namespace AqualinkAutomate::Serial::RFC2217
