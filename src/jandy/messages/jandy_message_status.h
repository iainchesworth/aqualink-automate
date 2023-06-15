#pragma once

#include <cstdint>
#include <string>
#include <span>
#include <vector>

#include "interfaces/imessagesignal_recv.h"
#include "jandy/config/jandy_config_auxillary.h"
#include "jandy/config/jandy_config_heater.h"
#include "jandy/config/jandy_config_pump.h"
#include "jandy/factories/jandy_message_factory_registration.h"
#include "jandy/messages/jandy_message.h"

namespace AqualinkAutomate::Messages
{

	enum class ComboModes : uint8_t
	{
		Pool = 0x00,
		Spa = 0x01,
		Unknown
	};

	class JandyMessage_Status : public JandyMessage, public Interfaces::IMessageSignalRecv<JandyMessage_Status>
	{
		static const uint8_t STATUS_PAYLOAD_LENGTH = 5;

	public:
		JandyMessage_Status();
		virtual ~JandyMessage_Status();

	public:
		ComboModes Mode() const;
		Config::PumpStatus FilterPump() const;
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
		virtual bool SerializeContents(std::vector<uint8_t>& message_bytes) const override;
		virtual bool DeserializeContents(const std::vector<uint8_t>& message_bytes) override;

	private:
		ComboModes m_Mode;
		Config::PumpStatus m_FilterPump;
		Config::AuxillaryStates m_Aux1;
		Config::AuxillaryStates m_Aux2;
		Config::AuxillaryStates m_Aux3;
		Config::AuxillaryStates m_Aux4;
		Config::AuxillaryStates m_Aux5;
		Config::AuxillaryStates m_Aux6;
		Config::AuxillaryStates m_Aux7;
		Config::HeaterStatus m_PoolHeater;
		Config::HeaterStatus m_SolarHeater;
		Config::HeaterStatus m_SpaHeater;

	private:
		static const Factory::JandyMessageRegistration<Messages::JandyMessage_Status> g_JandyMessage_Status_Registration;
	};

}
// namespace AqualinkAutomate::Messages
