#pragma once

#include <cstdint>
#include <string>
#include <span>
#include <vector>

#include "interfaces/imessagesignal_recv.h"
#include "jandy/config/jandy_config_auxillary.h"
#include "jandy/config/jandy_config_heater.h"
#include "jandy/factories/jandy_message_factory_registration.h"
#include "jandy/messages/jandy_message.h"

namespace AqualinkAutomate::Messages
{

	enum class ComboModes : uint8_t
	{
		Pool = 0x00,
		Spa = 0x01
	};

	enum class EquipmentModes : uint8_t
	{
		Off = 0x00,
		Running = 0x01
	};

	namespace Payload
	{
		struct JandyMessage_Status_Payload_Byte0
		{
			bool : 1;
			Config::AuxillaryStates Aux2 : 1;
			bool : 1;
			Config::AuxillaryStates Aux3 : 1;
			bool : 3;
			Config::AuxillaryStates Aux7 : 1;
		};

		struct JandyMessage_Status_Payload_Byte1
		{
			Config::AuxillaryStates Aux5 : 1;
			bool : 2;
			EquipmentModes FilterPump : 1;
			bool : 1;
			ComboModes SpaMode : 1;
			bool : 1;
			Config::AuxillaryStates Aux1 : 1;
		};

		struct JandyMessage_Status_Payload_Byte2
		{
			bool : 1;
			Config::AuxillaryStates Aux6 : 1;
			bool : 5;
			Config::AuxillaryStates Aux4 : 1;
		};

		struct JandyMessage_Status_Payload_Byte3
		{
			bool : 1;
			Config::HeaterStatus PoolHeater : 3;
			bool : 4;
		};

		struct JandyMessage_Status_Payload_Byte4
		{
			bool : 1;
			Config::HeaterStatus SolarHeater : 3;
			bool : 1;
			Config::HeaterStatus SpaHeater : 3;
		};
	}
	// namespace Payload

	class JandyMessage_Status : public JandyMessage, public Interfaces::IMessageSignalRecv<JandyMessage_Status>
	{
		static const uint8_t STATUS_PAYLOAD_LENGTH = 5;

	public:
		JandyMessage_Status();
		virtual ~JandyMessage_Status();

	public:
		ComboModes Mode() const;
		EquipmentModes FilterPump() const;
		Config::AuxillaryStates Aux1() const;
		Config::AuxillaryStates Aux2() const;
		Config::AuxillaryStates Aux3() const;
		Config::AuxillaryStates Aux4() const;
		Config::AuxillaryStates Aux5() const;
		Config::AuxillaryStates Aux6() const;
		Config::AuxillaryStates Aux7() const;
		Config::HeaterStatus PoolHeater() const;
		Config::HeaterStatus SpaHeater() const;
		Config::HeaterStatus SolarHeater() const;

	public:
		virtual std::string ToString() const override;

	public:
		virtual void Serialize(std::vector<uint8_t>& message_bytes) const override;
		virtual void Deserialize(const std::span<const std::byte>& message_bytes) override;

	private:
		Payload::JandyMessage_Status_Payload_Byte0 m_Payload_Byte0;
		Payload::JandyMessage_Status_Payload_Byte1 m_Payload_Byte1;
		Payload::JandyMessage_Status_Payload_Byte2 m_Payload_Byte2;
		Payload::JandyMessage_Status_Payload_Byte3 m_Payload_Byte3;
		Payload::JandyMessage_Status_Payload_Byte4 m_Payload_Byte4;

	private:
		static const Factory::JandyMessageRegistration<Messages::JandyMessage_Status> g_JandyMessage_Status_Registration;
	};

}
// namespace AqualinkAutomate::Messages
