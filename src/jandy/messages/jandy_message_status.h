#pragma once

#include <cstdint>
#include <string>
#include <span>
#include <vector>

#include "interfaces/imessagesignal_recv.h"
#include "kernel/auxillary_devices/auxillary_states.h"
#include "kernel/auxillary_devices/heater_status.h"
#include "kernel/auxillary_devices/pump_status.h"
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
		Kernel::PumpStatuses FilterPump() const;
		Kernel::AuxillaryStatuses Aux1() const;
		Kernel::AuxillaryStatuses Aux2() const;
		Kernel::AuxillaryStatuses Aux3() const;
		Kernel::AuxillaryStatuses Aux4() const;
		Kernel::AuxillaryStatuses Aux5() const;
		Kernel::AuxillaryStatuses Aux6() const;
		Kernel::AuxillaryStatuses Aux7() const;
		Kernel::HeaterStatuses PoolHeater() const;
		Kernel::HeaterStatuses SpaHeater() const;
		Kernel::HeaterStatuses SolarHeater() const;

	public:
		virtual std::string ToString() const override;

	public:
		virtual bool SerializeContents(std::vector<uint8_t>& message_bytes) const override;
		virtual bool DeserializeContents(const std::vector<uint8_t>& message_bytes) override;

	private:
		ComboModes m_Mode;
		Kernel::PumpStatuses m_FilterPump;
		Kernel::AuxillaryStatuses m_Aux1;
		Kernel::AuxillaryStatuses m_Aux2;
		Kernel::AuxillaryStatuses m_Aux3;
		Kernel::AuxillaryStatuses m_Aux4;
		Kernel::AuxillaryStatuses m_Aux5;
		Kernel::AuxillaryStatuses m_Aux6;
		Kernel::AuxillaryStatuses m_Aux7;
		Kernel::HeaterStatuses m_PoolHeater;
		Kernel::HeaterStatuses m_SolarHeater;
		Kernel::HeaterStatuses m_SpaHeater;

	private:
		static const Factory::JandyMessageRegistration<Messages::JandyMessage_Status> g_JandyMessage_Status_Registration;
	};

}
// namespace AqualinkAutomate::Messages
