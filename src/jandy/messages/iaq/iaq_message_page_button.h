#pragma once

#include <cstddef>
#include <string>
#include <span>
#include <string>
#include <vector>

#include "interfaces/imessagesignal_recv.h"
#include "jandy/factories/jandy_message_factory_registration.h"
#include "jandy/messages/iaq/iaq_message.h"

namespace AqualinkAutomate::Messages
{
	enum class ButtonStatuses : uint8_t
	{
		Off = 0x00,
		On = 0x01,
		Enabled = 0x02,
		Unknown = 0xFF
	};

	enum class ButtonTypes : uint8_t
	{
		None = 0x00,
		Generic = 0x01,
		Cooling = 0x02,
		Waterfall = 0x03,
		Fountain = 0x04,
		Heating = 0x05,
		HeatingAndCooling = 0x06,
		Light = 0x07,
		Filtration = 0x08,
		SolarHeating = 0x09,
		DeckJet = 0x0A,
		HeaterGreen = 0x0B,
		Unknown = 0xFF
	};

	class IAQMessage_PageButton : public IAQMessage, public Interfaces::IMessageSignalRecv<IAQMessage_PageButton>
	{
	public:
		static const uint8_t Index_ButtonState = 5;
		// Unknown byte value stored at index position 6.
		static const uint8_t Index_ButtonType = 7;
		static const uint8_t Index_ButtonNameText = 8;

	public:
		IAQMessage_PageButton();
		virtual ~IAQMessage_PageButton();

	public:
		ButtonStatuses ButtonStatus() const;
		ButtonTypes ButtonType() const;
		std::string ButtonName() const;

	public:
		virtual std::string ToString() const override;

	public:
		virtual bool SerializeContents(std::vector<uint8_t>& message_bytes) const override;
		virtual bool DeserializeContents(const std::vector<uint8_t>& message_bytes) override;

	private:
		ButtonStatuses m_ButtonStatus;
		ButtonTypes m_ButtonType;
		std::string m_ButtonName;

	private:
		static const Factory::JandyMessageRegistration<Messages::IAQMessage_PageButton> g_IAQMessage_PageButton_Registration;
	};

}
// namespace AqualinkAutomate::Messages
