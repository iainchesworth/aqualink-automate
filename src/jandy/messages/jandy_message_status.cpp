#include <format>

#include <magic_enum.hpp>

#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/jandy_message_status.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Messages
{
	const uint8_t JandyMessage_Status::STATUS_PAYLOAD_LENGTH{ 5 };

	const Factory::JandyMessageRegistration<Messages::JandyMessage_Status> JandyMessage_Status::g_JandyMessage_Status_Registration(JandyMessageIds::Status);

	JandyMessage_Status::JandyMessage_Status() :
		JandyMessage(JandyMessageIds::Status),
		Interfaces::IMessageSignalRecv<JandyMessage_Status>(), 
		m_Mode(ComboModes::Unknown),
		m_FilterPump(Kernel::PumpStatus::Unknown),
		m_Aux1(Kernel::AuxillaryStatuses::Unknown),
		m_Aux2(Kernel::AuxillaryStatuses::Unknown),
		m_Aux3(Kernel::AuxillaryStatuses::Unknown),
		m_Aux4(Kernel::AuxillaryStatuses::Unknown),
		m_Aux5(Kernel::AuxillaryStatuses::Unknown),
		m_Aux6(Kernel::AuxillaryStatuses::Unknown),
		m_Aux7(Kernel::AuxillaryStatuses::Unknown),
		m_PoolHeater(Kernel::HeaterStatus::Unknown),
		m_SolarHeater(Kernel::HeaterStatus::Unknown),
		m_SpaHeater(Kernel::HeaterStatus::Unknown)
	{
	}

	JandyMessage_Status::~JandyMessage_Status()
	{
	}

	ComboModes JandyMessage_Status::Mode() const
	{
		return m_Mode;
	}

	Kernel::PumpStatus JandyMessage_Status::FilterPump() const
	{
		return m_FilterPump;
	}

	Kernel::AuxillaryStatuses JandyMessage_Status::Aux1() const
	{
		return m_Aux1;
	}

	Kernel::AuxillaryStatuses JandyMessage_Status::Aux2() const
	{
		return m_Aux2;
	}

	Kernel::AuxillaryStatuses JandyMessage_Status::Aux3() const
	{
		return m_Aux3;
	}

	Kernel::AuxillaryStatuses JandyMessage_Status::Aux4() const
	{
		return m_Aux4;
	}

	Kernel::AuxillaryStatuses JandyMessage_Status::Aux5() const
	{
		return m_Aux5;
	}

	Kernel::AuxillaryStatuses JandyMessage_Status::Aux6() const
	{
		return m_Aux6;
	}

	Kernel::AuxillaryStatuses JandyMessage_Status::Aux7() const
	{
		return m_Aux7;
	}

	Kernel::HeaterStatus JandyMessage_Status::PoolHeater() const
	{
		return m_PoolHeater;
	}

	Kernel::HeaterStatus JandyMessage_Status::SpaHeater() const
	{
		return m_SpaHeater;
	}

	Kernel::HeaterStatus JandyMessage_Status::SolarHeater() const
	{
		return m_SolarHeater;
	}

	std::string JandyMessage_Status::ToString() const
	{
		return std::format("Packet: {} || Payload: {}", JandyMessage::ToString(), 0);
	}

	bool JandyMessage_Status::SerializeContents(std::vector<uint8_t>& message_bytes) const
	{
		LogTrace(Channel::Messages, std::format("Serialising JandyMessage_Status type into {} bytes", message_bytes.size()));

		// Insert 5 NUL bytes to ensure that the payload is the correct length.
		// Note that there is no actual computation of the correct content here.

		message_bytes.push_back(0x00);
		message_bytes.push_back(0x00);
		message_bytes.push_back(0x00);
		message_bytes.push_back(0x00);
		message_bytes.push_back(0x00);

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

			m_Aux5 = magic_enum::enum_cast<Kernel::AuxillaryStatuses>((message_bytes[5] & 0x80) >> 7).value_or(Kernel::AuxillaryStatuses::Unknown);
			m_Aux2 = magic_enum::enum_cast<Kernel::AuxillaryStatuses>((message_bytes[5] & 0x40) >> 6).value_or(Kernel::AuxillaryStatuses::Unknown);
			m_Aux3 = magic_enum::enum_cast<Kernel::AuxillaryStatuses>((message_bytes[5] & 0x10) >> 4).value_or(Kernel::AuxillaryStatuses::Unknown);
			m_Aux7 = magic_enum::enum_cast<Kernel::AuxillaryStatuses>((message_bytes[5] & 0x01) >> 0).value_or(Kernel::AuxillaryStatuses::Unknown);

			m_FilterPump = magic_enum::enum_cast<Kernel::PumpStatus>((message_bytes[6] & 0x10) >> 4).value_or(Kernel::PumpStatus::Unknown);
			m_Mode = magic_enum::enum_cast<ComboModes>((message_bytes[6] & 0x04) >> 2).value_or(ComboModes::Unknown);
			m_Aux1 = magic_enum::enum_cast<Kernel::AuxillaryStatuses>((message_bytes[6] & 0x01) >> 0).value_or(Kernel::AuxillaryStatuses::Unknown);

			uint8_t tempx = ((message_bytes[7] & 0x40) >> 6); // ENA?
			uint8_t tempy = ((message_bytes[8] & 0x04) >> 2); // ENA?

			m_Aux6 = magic_enum::enum_cast<Kernel::AuxillaryStatuses>((message_bytes[7] & 0x40) >> 6).value_or(Kernel::AuxillaryStatuses::Unknown);
			m_Aux4 = magic_enum::enum_cast<Kernel::AuxillaryStatuses>((message_bytes[7] & 0x01) >> 0).value_or(Kernel::AuxillaryStatuses::Unknown);
			
			m_PoolHeater = magic_enum::enum_cast<Kernel::HeaterStatus>((message_bytes[8] & 0x70) >> 4).value_or(Kernel::HeaterStatus::Unknown);

			m_SolarHeater = magic_enum::enum_cast<Kernel::HeaterStatus>((message_bytes[9] & 0x70) >> 4).value_or(Kernel::HeaterStatus::Unknown);
			m_SpaHeater = magic_enum::enum_cast<Kernel::HeaterStatus>((message_bytes[9] & 0x07) >> 0).value_or(Kernel::HeaterStatus::Unknown);

			LogInfo(
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
