#include <format>

#include "logging/logging.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/jandy_message_status.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	const Factory::JandyMessageRegistration<Messages::JandyMessage_Status> JandyMessage_Status::g_JandyMessage_Status_Registration(JandyMessageIds::Status);

	JandyMessage_Status::JandyMessage_Status() :
		JandyMessage(JandyMessageIds::Status),
		Interfaces::IMessageSignal<JandyMessage_Status>(),
		m_Payload_Byte0(),
		m_Payload_Byte1(),
		m_Payload_Byte2(),
		m_Payload_Byte3(),
		m_Payload_Byte4()
	{
	}

	JandyMessage_Status::~JandyMessage_Status()
	{
	}

	ComboModes JandyMessage_Status::Mode() const
	{
		return m_Payload_Byte1.SpaMode;
	}

	EquipmentModes JandyMessage_Status::FilterPump() const
	{
		return m_Payload_Byte1.FilterPump;
	}

	AuxStatus JandyMessage_Status::Aux1() const
	{
		return m_Payload_Byte1.Aux1;
	}

	AuxStatus JandyMessage_Status::Aux2() const
	{
		return m_Payload_Byte0.Aux2;
	}

	AuxStatus JandyMessage_Status::Aux3() const
	{
		return m_Payload_Byte0.Aux3;
	}

	AuxStatus JandyMessage_Status::Aux4() const
	{
		return m_Payload_Byte2.Aux4;
	}

	AuxStatus JandyMessage_Status::Aux5() const
	{
		return m_Payload_Byte1.Aux5;
	}

	AuxStatus JandyMessage_Status::Aux6() const
	{
		return m_Payload_Byte2.Aux6;
	}

	AuxStatus JandyMessage_Status::Aux7() const
	{
		return m_Payload_Byte0.Aux7;
	}

	HeaterStatus JandyMessage_Status::PoolHeater() const
	{
		return m_Payload_Byte3.PoolHeater;
	}

	HeaterStatus JandyMessage_Status::SpaHeater() const
	{
		return m_Payload_Byte4.SpaHeater;
	}

	HeaterStatus JandyMessage_Status::SolarHeater() const
	{
		return m_Payload_Byte4.SolarHeater;
	}

	std::string JandyMessage_Status::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", JandyMessage::ToString(), 0);
	}

	void JandyMessage_Status::Serialize(std::span<const std::byte>& message_bytes) const
	{
	}

	void JandyMessage_Status::Deserialize(const std::span<const std::byte>& message_bytes)
	{
		if (!PacketIsValid(message_bytes))
		{
			LogWarning(Channel::Messages, "Failed during JandyMessage_Status deserialising; packet was not validly formatted");
		}
		else if (JandyMessage::Deserialize(message_bytes); STATUS_PAYLOAD_LENGTH != m_Payload.size())
		{
			LogWarning(Channel::Messages, std::format("Failed during JandyMessage_Status deserialising; payload size mismatch: {} vs {}", STATUS_PAYLOAD_LENGTH, m_Payload.size()));
		}
		else
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} payload bytes from span into JandyMessage_Status type", m_Payload.size()));

			m_Payload_Byte0 = *(reinterpret_cast<Payload::JandyMessage_Status_Payload_Byte0*>(&(m_Payload[0])));
			m_Payload_Byte1 = *(reinterpret_cast<Payload::JandyMessage_Status_Payload_Byte1*>(&(m_Payload[1])));
			m_Payload_Byte2 = *(reinterpret_cast<Payload::JandyMessage_Status_Payload_Byte2*>(&(m_Payload[2])));
			m_Payload_Byte3 = *(reinterpret_cast<Payload::JandyMessage_Status_Payload_Byte3*>(&(m_Payload[3])));
			m_Payload_Byte4 = *(reinterpret_cast<Payload::JandyMessage_Status_Payload_Byte4*>(&(m_Payload[4])));

			LogTrace(
				Channel::Messages, 
				std::format(
					"Status Flags -> ({} of {} bytes): (0x{:02x}) {:08B} (0x{:02x}) {:08B} (0x{:02x}) {:08B} (0x{:02x}) {:08B} (0x{:02x}) {:08B}", 
					m_Payload.size(),
					message_bytes.size_bytes(), 
					m_Payload[0], 
					m_Payload[0],
					m_Payload[1], 
					m_Payload[1],
					m_Payload[2], 
					m_Payload[2],
					m_Payload[3], 
					m_Payload[3],
					m_Payload[4], 
					m_Payload[4]
				)
			);
		}
	}

}
// namespace AqualinkAutomate::Messages
