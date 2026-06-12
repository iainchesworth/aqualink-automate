#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "interfaces/imessagesignal_recv.h"
#include "messages/pentair_message.h"

namespace AqualinkAutomate::Pentair::Messages
{

	// IntelliCenter / EasyTouch controller status broadcast (CMD 0x02).
	//
	// The controller periodically broadcasts the system state.  The DATA fields
	// the application decodes (offsets relative to the start of DATA):
	//
	//   DATA[0] clock hour (0-23)
	//   DATA[1] clock minute (0-59)
	//   DATA[2] equipment circuit bitmask (bit0 spa, bit5 pool, ...)
	//   DATA[3] pool / water temperature (degrees Fahrenheit)
	//   DATA[4] air temperature (degrees Fahrenheit)
	//   DATA[5] heater-active bitmask (bit0 pool heater, bit1 spa heater)
	//
	// A status frame with fewer DATA bytes leaves the missing fields at their
	// defaults and Has*() returns false for fields that were not present.
	class PentairControllerMessage_Status : public PentairMessage, public Interfaces::IMessageSignalRecv<PentairControllerMessage_Status>
	{
	public:
		static constexpr uint8_t Data_Index_Hour = 0;
		static constexpr uint8_t Data_Index_Minute = 1;
		static constexpr uint8_t Data_Index_CircuitMask = 2;
		static constexpr uint8_t Data_Index_WaterTempF = 3;
		static constexpr uint8_t Data_Index_AirTempF = 4;
		static constexpr uint8_t Data_Index_HeaterMask = 5;

		// Circuit bitmask bits.
		static constexpr uint8_t CIRCUIT_SPA = 0x01;
		static constexpr uint8_t CIRCUIT_POOL = 0x20;

		// Heater bitmask bits.
		static constexpr uint8_t HEATER_POOL = 0x01;
		static constexpr uint8_t HEATER_SPA = 0x02;

	public:
		PentairControllerMessage_Status() noexcept;
		~PentairControllerMessage_Status() override = default;

	public:
		uint8_t Hour() const;
		uint8_t Minute() const;

		bool SpaCircuitOn() const;
		bool PoolCircuitOn() const;

		bool HasWaterTemp() const;
		uint8_t WaterTempF() const;

		bool HasAirTemp() const;
		uint8_t AirTempF() const;

		bool PoolHeaterOn() const;
		bool SpaHeaterOn() const;

	public:
		std::string ToString() const override;

	public:
		bool SerializeContents(std::vector<uint8_t>& message_bytes) const override;
		bool DeserializeContents(std::span<const uint8_t> message_bytes) override;

	private:
		uint8_t m_Hour;
		uint8_t m_Minute;
		uint8_t m_CircuitMask;
		uint8_t m_WaterTempF;
		uint8_t m_AirTempF;
		uint8_t m_HeaterMask;
		bool m_HasWaterTemp;
		bool m_HasAirTemp;
	};

}
// namespace AqualinkAutomate::Pentair::Messages
