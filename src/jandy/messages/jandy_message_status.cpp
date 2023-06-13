#include <format>

#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/jandy_message_status.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{

	const Factory::JandyMessageRegistration<Messages::JandyMessage_Status> JandyMessage_Status::g_JandyMessage_Status_Registration(JandyMessageIds::Status);

	JandyMessage_Status::JandyMessage_Status() :
		JandyMessage(JandyMessageIds::Status),
		Interfaces::IMessageSignalRecv<JandyMessage_Status>(),
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

	Config::AuxillaryStates JandyMessage_Status::Aux1() const
	{
		return m_Payload_Byte1.Aux1;
	}

	Config::AuxillaryStates JandyMessage_Status::Aux2() const
	{
		return m_Payload_Byte0.Aux2;
	}

	Config::AuxillaryStates JandyMessage_Status::Aux3() const
	{
		return m_Payload_Byte0.Aux3;
	}

	Config::AuxillaryStates JandyMessage_Status::Aux4() const
	{
		return m_Payload_Byte2.Aux4;
	}

	Config::AuxillaryStates JandyMessage_Status::Aux5() const
	{
		return m_Payload_Byte1.Aux5;
	}

	Config::AuxillaryStates JandyMessage_Status::Aux6() const
	{
		return m_Payload_Byte2.Aux6;
	}

	Config::AuxillaryStates JandyMessage_Status::Aux7() const
	{
		return m_Payload_Byte0.Aux7;
	}

	Config::HeaterStatus JandyMessage_Status::PoolHeater() const
	{
		return m_Payload_Byte3.PoolHeater;
	}

	Config::HeaterStatus JandyMessage_Status::SpaHeater() const
	{
		return m_Payload_Byte4.SpaHeater;
	}

	Config::HeaterStatus JandyMessage_Status::SolarHeater() const
	{
		return m_Payload_Byte4.SolarHeater;
	}

	std::string JandyMessage_Status::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", JandyMessage::ToString(), 0);
	}

	bool JandyMessage_Status::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		LogTrace(Channel::Messages, std::format("Serialising JandyMessage_Status type into {} bytes", message_bytes.size()));

		return true;
	}

	bool JandyMessage_Status::DeserializeContents(const std::vector<uint8_t>& message_bytes)
	{
		LogTrace(Channel::Messages, std::format("Deserialising {} bytes into JandyMessage_Status type", message_bytes.size()));

		if ((JandyMessage::MINIMUM_PACKET_LENGTH + STATUS_PAYLOAD_LENGTH) != message_bytes.size())
		{
			LogWarning(Channel::Messages, std::format("Failed during JandyMessage_Status deserialising; payload size mismatch: {} vs {}", STATUS_PAYLOAD_LENGTH, message_bytes.size()));
		}
		else
		{
			LogTrace(Channel::Messages, std::format("Deserialising {} payload bytes from span into JandyMessage_Status type", message_bytes.size()));

			///FIXME 
			/*m_Payload_Byte0 = Payload::JandyMessage_Status_Payload_Byte0(message_bytes[5]);
			m_Payload_Byte1 = Payload::JandyMessage_Status_Payload_Byte1(message_bytes[6]);
			m_Payload_Byte2 = Payload::JandyMessage_Status_Payload_Byte2(message_bytes[7]);
			m_Payload_Byte3 = Payload::JandyMessage_Status_Payload_Byte3(message_bytes[8]);
			m_Payload_Byte4 = Payload::JandyMessage_Status_Payload_Byte4(message_bytes[9]);*/

			LogTrace(
				Channel::Messages, 
				std::format(
					"Status Flags -> ({} of {} bytes): (0x{:02x}) {:08B} (0x{:02x}) {:08B} (0x{:02x}) {:08B} (0x{:02x}) {:08B} (0x{:02x}) {:08B}", 
					message_bytes.size(),
					message_bytes.size(), 
					message_bytes[5],
					message_bytes[5],
					message_bytes[6],
					message_bytes[6],
					message_bytes[7],
					message_bytes[7],
					message_bytes[8],
					message_bytes[8],
					message_bytes[9],
					message_bytes[9]
				)
			);

			return true;
		}

		return false;
	}

}
// namespace AqualinkAutomate::Messages
