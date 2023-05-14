#pragma once

#include <cstdint>
#include <functional>

#include <boost/signals2.hpp>

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
		FilteredSlot_ByDeviceId(HandlerType handler, uint8_t device_id) :
			m_Handler(handler),
			m_DeviceId(device_id)
		{
			auto filtered_slot_handler = [this](const MESSAGE_TYPE& msg) -> void
			{
				if (m_DeviceId != msg.Destination().Raw())
				{
					// Message was not for this destination.
				}
				else
				{
					m_Handler(msg);
				}
			};

			m_Connection = typename MESSAGE_TYPE::GetSignal()->connect(filtered_slot_handler);
		}

		~FilteredSlot_ByDeviceId()
		{
			m_Connection.disconnect();
		}

	private:
		HandlerType m_Handler;
		uint8_t m_DeviceId;
		boost::signals2::connection m_Connection;
	};

}
// namespace AqualinkAutomate::Utility
