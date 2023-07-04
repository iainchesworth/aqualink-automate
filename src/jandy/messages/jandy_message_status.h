#pragma once

#include <cstdint>
#include <string>
#include <span>
#include <vector>

#include "interfaces/imessagesignal_recv.h"
#include "kernel/auxillary.h"
#include "kernel/heater.h"
#include "kernel/pump.h"
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
		static const uint8_t STATUS_PAYLOAD_LENGTH;

	public:
		JandyMessage_Status();
		virtual ~JandyMessage_Status();

	public:
		ComboModes Mode() const;
		Kernel::PumpStatus FilterPump() const;
		Kernel::AuxillaryStates Aux1() const;
		Kernel::AuxillaryStates Aux2() const;
		Kernel::AuxillaryStates Aux3() const;
		Kernel::AuxillaryStates Aux4() const;
		Kernel::AuxillaryStates Aux5() const;
		Kernel::AuxillaryStates Aux6() const;
		Kernel::AuxillaryStates Aux7() const;
		Kernel::HeaterStatus PoolHeater() const;
		Kernel::HeaterStatus SpaHeater() const;
		Kernel::HeaterStatus SolarHeater() const;

	public:
		virtual std::string ToString() const override;

	public:
		virtual bool SerializeContents(std::vector<uint8_t>& message_bytes) const override;
		virtual bool DeserializeContents(const std::vector<uint8_t>& message_bytes) override;

	private:
		ComboModes m_Mode;
		Kernel::PumpStatus m_FilterPump;
		Kernel::AuxillaryStates m_Aux1;
		Kernel::AuxillaryStates m_Aux2;
		Kernel::AuxillaryStates m_Aux3;
		Kernel::AuxillaryStates m_Aux4;
		Kernel::AuxillaryStates m_Aux5;
		Kernel::AuxillaryStates m_Aux6;
		Kernel::AuxillaryStates m_Aux7;
		Kernel::HeaterStatus m_PoolHeater;
		Kernel::HeaterStatus m_SolarHeater;
		Kernel::HeaterStatus m_SpaHeater;

	private:
		static const Factory::JandyMessageRegistration<Messages::JandyMessage_Status> g_JandyMessage_Status_Registration;
	};

}
// namespace AqualinkAutomate::Messages
