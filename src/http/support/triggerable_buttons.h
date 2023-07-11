#pragma once

#include <initializer_list>
#include <memory>
#include <vector>

#include <mstch/mstch.hpp>

#include "http/support/triggerable_button.h"

namespace AqualinkAutomate::HTTP
{

	class TriggerableButtons : public mstch::object
	{
	public:
		TriggerableButtons(std::initializer_list<std::shared_ptr<TriggerableButton>> triggerable_buttons) :
			mstch::object(),
			m_TriggerableButtons(triggerable_buttons)
		{
			register_methods(this,
				{
					{ "triggerable_buttons", &TriggerableButtons::triggerable_buttons }
				}
			);
		}

	public:
		mstch::node triggerable_buttons()
		{
			mstch::array button_array;
			
			for (auto elem : m_TriggerableButtons)
			{
				button_array.push_back(elem);
			}

			return button_array;
		}

	public:
		std::vector<std::shared_ptr<TriggerableButton>>& operator()()
		{
			return m_TriggerableButtons;
		}

	private:
		std::vector<std::shared_ptr<TriggerableButton>> m_TriggerableButtons;
	};

}
// namespace AqualinkAutomate::HTTP
