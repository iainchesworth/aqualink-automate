#pragma once

#include <functional>

#include <mstch/mstch.hpp>
#include <nlohmann/json.hpp>

namespace AqualinkAutomate::HTTP
{

	class TriggerableButton : public mstch::object
	{
		using ButtonId = std::string;
		using StatusFunc = std::function<nlohmann::json()>;
		using TriggerFunc = std::function<nlohmann::json(const nlohmann::json payload)>;

	public:
		TriggerableButton(ButtonId id, StatusFunc status_func, TriggerFunc trigger_func) :
			mstch::object()
		{
			register_methods(this,
				{
					{ "triggerable_button_id", &TriggerableButton::triggerable_button_id },
					{ "triggerable_button_label", &TriggerableButton::triggerable_button_label },
					{ "triggerable_button_status_id", &TriggerableButton::triggerable_button_status_id },
					{ "triggerable_button_status", &TriggerableButton::triggerable_button_status }
				}
			);
		}

	public:
		mstch::node triggerable_button_id()
		{
			return std::string{ Id() };
		}

		mstch::node triggerable_button_label()
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
