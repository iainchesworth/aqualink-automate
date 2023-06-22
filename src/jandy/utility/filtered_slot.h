#pragma once

#include <cstdint>
#include <functional>

#include <boost/signals2.hpp>

#include "jandy/devices/jandy_device_id.h"

namespace AqualinkAutomate::Utility
{

	class FilteredSlotBase
	{
	};

	template<typename MESSAGE_TYPE>
	class FilteredSlot_ByDeviceId : public FilteredSlotBase
	{
	public:
		using HandlerType = std::function<void(const MESSAGE_TYPE&)>;

	public:
		FilteredSlot_ByDeviceId(HandlerType handler, Devices::JandyDeviceId device_id) :
			m_Handler(handler),
			m_DeviceId(device_id)
		{
			auto filtered_slot_handler = [this](const MESSAGE_TYPE& msg) -> void
			{
				if (m_DeviceId != msg.Destination().Id())
				{
					// Message was not for this destination.
				}
				else
				{
					m_Handler(msg);
				}
			};

			m_Connection = MESSAGE_TYPE::GetSignal()->connect(filtered_slot_handler);
		}

		~FilteredSlot_ByDeviceId()
		{
			m_Connection.disconnect();
		}

	private:
		HandlerType m_Handler;
		Devices::JandyDeviceId m_DeviceId;
		boost::signals2::connection m_Connection;
	};

}
// namespace AqualinkAutomate::Utility
