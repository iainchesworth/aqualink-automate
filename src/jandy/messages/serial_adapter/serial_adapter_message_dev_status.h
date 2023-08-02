#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <span>
#include <variant>
#include <vector>

#include "interfaces/imessagesignal_recv.h"
#include "jandy/auxillaries/jandy_auxillary_id.h"
#include "jandy/auxillaries/jandy_auxillary_status.h"
#include "jandy/factories/jandy_message_factory_registration.h"
#include "jandy/messages/serial_adapter/serial_adapter_message.h"
#include "kernel/temperature.h"
#include "types/units_electric_potential.h"

namespace AqualinkAutomate::Messages
{

	enum class SerialAdapter_CommandTypes
	{
		Query = 0x05,
		Toggle = 0xFF,	// Actually Unknown Value
		SetOff = 0x80,
		SetOn = 0x81,
		Action = 0xFF,	// Actually Unknown Value
		Unknown = 0xFF
	};

	enum class SerialAdapter_ConfigControlCommands
	{
		ECHO = 0xFF,	// Actually Unknown Value
		RSPFMT = 0xFF,	// Actually Unknown Value
		RST = 0xFF,		// Actually Unknown Value
		VERS = 0xFF,	// Actually Unknown Value
		DIAG = 0xFF,	// Actually Unknown Value
		S1 = 0xFF,		// Actually Unknown Value
		CMDCHR = 0xFF,	// Actually Unknown Value
		NRMCHR = 0xFF,	// Actually Unknown Value
		ERRCHR = 0xFF,	// Actually Unknown Value
		COSMSGS = 0xFF	// Actually Unknown Value
	};

	enum class SerialAdapter_SystemConfigurationStatuses
	{
		MODEL = 0x00,
		OPTIONS = 0x01,
		OPMODE = 0x0D,
		VBAT = 0x0E,
		LEDS = 0xFF		// Actually Unknown Value
	};

	enum class SerialAdapter_SCS_OpModes
	{
		Auto = 0x00,
		Service = 0x01,
		Timeout = 0x02,
		Unknown = 0xFF
	};

	struct SerialAdapter_SCS_Options
	{
		bool HasCleaner : 1;
		bool TwoSpeedPump : 1;
		bool HasSpillover : 1;
		bool HeaterCoolDownDisabled : 1;
		bool ServiceCalibrationMode : 1;
		bool SpareAuxOnWithFilterPumpAndSpa : 1;
		bool CommonHeaterForSpaAndPool : 1;
		bool ExtraDelayForHeatPump : 1;
	};

	struct SerialAdapter_SCS_BatteryCondition
	{
		bool IsLow;
		AqualinkAutomate::Units::voltage Voltage;
	};

	enum class SerialAdapter_SystemPumpCommands
	{
		PUMPLO = 0x0D,
		PUMP = 0x0C,
		CLEANR = 0x10,
		SPILLOVER = 0x0F,
		SPA = 0x0E
	};

	enum class SerialAdapter_SystemTemperatureCommands
	{
		UNITS = 0x0A,
		POOLHT = 0x11,
		POOLHT2 = 0x12,
		SPAHT = 0x13,
		SOLHT = 0x14,
		POOLSP = 0x05,
		POOLSP2 = 0x06,
		SPASP = 0x07,
		POOLTMP = 0x08,
		SPATMP = 0x0B,
		AIRTMP = 0x09,
		SOLTMP = 0x0C
	};

	enum class SerialAdapter_BAO_States
	{
		Off = 0x00,
		On = 0x01,
		Unknown = 0xFF
	};

	enum class SerialAdapter_ErrorCodes
	{
		AuxillaryUnavailable_OptionSwitchIsSet = 0x0E,
		AuxillaryUnavailable_NotInstalled = 0x11,
		Unknown = 0xFF
	};

	enum class SerialAdapter_UnknownCommands
	{
		ErrorOccurred = 0x02,
		Unknown = 0xFF
	};

	using SerialAdapter_StatusTypes = 
		std::variant<
			std::monostate,
			SerialAdapter_ConfigControlCommands, 
			SerialAdapter_SystemConfigurationStatuses, 
			SerialAdapter_SystemPumpCommands, 
			SerialAdapter_SystemTemperatureCommands, 
			Auxillaries::JandyAuxillaryIds, 
			SerialAdapter_UnknownCommands
		>;

	class SerialAdapterMessage_DevStatus : public SerialAdapterMessage, public Interfaces::IMessageSignalRecv<SerialAdapterMessage_DevStatus>
	{
		static const uint8_t Index_StatusType = 4;
		static const uint8_t Index_DeviceId = 7;
		static const uint8_t Index_ModelType_Part1 = 5;
		static const uint8_t Index_ModelType_Part2 = 6;
		static const uint8_t Index_OpMode = 6;
		static const uint8_t Index_Options = 6;
		static const uint8_t Index_BatteryVoltage_Part1 = 5;
		static const uint8_t Index_BatteryVoltage_Part2 = 6;
		static const uint8_t Index_TemperatureUnits = 6;
		static const uint8_t Index_PoolTemperature_SetPoint = 6;
		static const uint8_t Index_SpaTemperature_SetPoint = 6;
		static const uint8_t Index_AirTemperature = 6;
		static const uint8_t Index_PoolTemperature = 6;
		static const uint8_t Index_SolarTemperature = 6;
		static const uint8_t Index_SpaTemperature = 6;
		static const uint8_t Index_AuxState = 6;

	public:
		static const uint8_t SERIALADAPTER_AUX_ID_OFFSET = 0x14;

	public:
		SerialAdapterMessage_DevStatus();
		SerialAdapterMessage_DevStatus(const SerialAdapter_ConfigControlCommands  sa_ccc);
		SerialAdapterMessage_DevStatus(const SerialAdapter_SystemConfigurationStatuses sa_scs);
		SerialAdapterMessage_DevStatus(const SerialAdapter_SystemPumpCommands sa_spc);
		SerialAdapterMessage_DevStatus(const SerialAdapter_SystemTemperatureCommands sa_stc);
		SerialAdapterMessage_DevStatus(const Auxillaries::JandyAuxillaryIds sa_jai);
		virtual ~SerialAdapterMessage_DevStatus();

	public:
		std::optional<uint16_t> ModelType() const;
		std::optional<SerialAdapter_SCS_OpModes> OpMode() const;
		std::optional<SerialAdapter_SCS_Options> Options() const;
		std::optional<SerialAdapter_SCS_BatteryCondition> BatteryCondition() const;

	public:
		std::optional<Kernel::TemperatureUnits> TemperatureUnits() const;
		std::optional<Kernel::Temperature> Pool_SetPoint_One() const;
		std::optional<Kernel::Temperature> Pool_SetPoint_Two() const;
		std::optional<Kernel::Temperature> Spa_SetPoint() const;
		std::optional<Kernel::Temperature> AirTemperature() const;
		std::optional<Kernel::Temperature> PoolTemperature() const;
		std::optional<Kernel::Temperature> SolarTemperature() const;
		std::optional<Kernel::Temperature> SpaTemperature() const;

	public:
		std::optional<std::tuple<Auxillaries::JandyAuxillaryIds, std::optional<Auxillaries::JandyAuxillaryStatuses>>> AuxilliaryState() const;

	public:
		virtual std::string ToString() const override;

	public:
		virtual bool SerializeContents(std::vector<uint8_t>& message_bytes) const override;
		virtual bool DeserializeContents(const std::vector<uint8_t>& message_bytes) override;

	private:
		SerialAdapter_StatusTypes m_StatusType;

	private:
		std::optional<uint16_t> m_ModelType{ std::nullopt };

	private:
		std::optional<SerialAdapter_SCS_OpModes> m_OpMode{ std::nullopt };
		std::optional<SerialAdapter_SCS_Options> m_Options{ std::nullopt };
		std::optional<SerialAdapter_SCS_BatteryCondition> m_BatteryCondition{ std::nullopt };

	private:
		std::optional<Kernel::TemperatureUnits> m_TemperatureUnits{ std::nullopt };
		std::optional<Kernel::Temperature> m_PoolTemperature_SetPoint_One{ std::nullopt };
		std::optional<Kernel::Temperature> m_PoolTemperature_SetPoint_Two{ std::nullopt };
		std::optional<Kernel::Temperature> m_SpaTemperature_SetPoint{ std::nullopt };
		std::optional<Kernel::Temperature> m_AirTemperature{ std::nullopt };
		std::optional<Kernel::Temperature> m_PoolTemperature{ std::nullopt };
		std::optional<Kernel::Temperature> m_SolarTemperature{ std::nullopt };
		std::optional<Kernel::Temperature> m_SpaTemperature{ std::nullopt };

	private:
		std::optional<std::tuple<Auxillaries::JandyAuxillaryIds, std::optional<Auxillaries::JandyAuxillaryStatuses>>> m_Aux_State{ std::nullopt };

	private:
		std::optional<Auxillaries::JandyAuxillaryStatuses> m_Cleaner_State;
		std::optional<Auxillaries::JandyAuxillaryStatuses> m_Spillover_State;

	private:
		static const Factory::JandyMessageRegistration<Messages::SerialAdapterMessage_DevStatus> g_SerialAdapterMessage_DevStatus_Registration;
	};

}
// namespace AqualinkAutomate::Messages
