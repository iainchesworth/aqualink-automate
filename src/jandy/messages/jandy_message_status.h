#pragma once

#include <cstddef>
#include <string>
#include <span>
#include <vector>

#include "interfaces/imessagesignal_recv.h"
#include "jandy/factories/jandy_message_factory_registration.h"
#include "jandy/messages/jandy_message.h"

namespace AqualinkAutomate::Messages
{
	enum class AuxStatus : uint8_t
	{
		Off = 0x00,
		On = 0x01
	};

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

	enum class HeaterStatus : uint8_t
	{
		Off = 0x00,
		Heating = 0x01,
		Enabled = 0x04
	};

	namespace Payload
	{
		struct JandyMessage_Status_Payload_Byte0
		{
			bool : 1;
			AuxStatus Aux2 : 1;
			bool : 1;
			AuxStatus Aux3 : 1;
			bool : 3;
			AuxStatus Aux7 : 1;
		};

		struct JandyMessage_Status_Payload_Byte1
		{
			AuxStatus Aux5 : 1;
			bool : 2;
			EquipmentModes FilterPump : 1;
			bool : 1;
			ComboModes SpaMode : 1;
			bool : 1;
			AuxStatus Aux1 : 1;
		};

		struct JandyMessage_Status_Payload_Byte2
		{
			bool : 1;
			AuxStatus Aux6 : 1;
			bool : 5;
			AuxStatus Aux4 : 1;
		};

		struct JandyMessage_Status_Payload_Byte3
		{
			bool : 1;
			HeaterStatus PoolHeater : 3;
			bool : 4;
		};

		struct JandyMessage_Status_Payload_Byte4
		{
			bool : 1;
			HeaterStatus SolarHeater : 3;
			bool : 1;
			HeaterStatus SpaHeater : 3;
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
		AuxStatus Aux1() const;
		AuxStatus Aux2() const;
		AuxStatus Aux3() const;
		AuxStatus Aux4() const;
		AuxStatus Aux5() const;
		AuxStatus Aux6() const;
		AuxStatus Aux7() const;
		HeaterStatus PoolHeater() const;
		HeaterStatus SpaHeater() const;
		HeaterStatus SolarHeater() const;

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
