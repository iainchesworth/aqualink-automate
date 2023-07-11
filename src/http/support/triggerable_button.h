#pragma once

#include <functional>

#include <mstch/mstch.hpp>
#include <nlohmann/json.hpp>

namespace AqualinkAutomate::HTTP
{

	class TriggerableButton : public mstch::object
	{
	public:
		using ButtonId = std::string;
		using StatusFunc = std::function<std::string()>;
		using TriggerFunc = std::function<nlohmann::json(const nlohmann::json payload)>;

	public:
		TriggerableButton(ButtonId id, StatusFunc status, TriggerFunc trigger) :
			mstch::object()
		{
			register_methods(this,
				{
					{ "triggerable_button_name_id", &TriggerableButton::triggerable_button_text_id },
					{ "triggerable_button_name", &TriggerableButton::triggerable_button_text },
					{ "triggerable_button_status_id", &TriggerableButton::triggerable_button_status_id },
					{ "triggerable_button_status", &TriggerableButton::triggerable_button_status }
				}
			);
		}

	public:
		mstch::node triggerable_button_text_id()
		{
			return std::string{ Id() };
		}

		mstch::node triggerable_button_text()
		{
			return std::string{ Id() };
		}

		mstch::node triggerable_button_status_id()
		{
			return std::string{ "Status()" };
		}

		mstch::node triggerable_button_status()
		{
			return std::string{ Status() };
		}

	public:
		const ButtonId Id() const
		{
			return m_Id;
		}

		std::string Status() const
		{
			return m_Status();
		}

		nlohmann::json Trigger(const nlohmann::json payload) const
		{
			return m_Trigger(payload);
		}

	private:
		ButtonId m_Id;
		StatusFunc m_Status;
		TriggerFunc m_Trigger;
	};

}
// namespace AqualinkAutomate::HTTP
