#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <variant>
#include <vector>

#include <boost/signals2.hpp>

#include "jandy/utility/filtered_slot.h"

namespace AqualinkAutomate::Utility
{
	
	class SlotConnectionManager
	{
		using ConnectionVariant = std::variant<boost::signals2::connection, std::shared_ptr<FilteredSlotBase>>;

	public:
		SlotConnectionManager();
		~SlotConnectionManager();

	public:
		template<typename MESSAGE_TYPE>
		bool RegisterSlot(std::function<void(const MESSAGE_TYPE& msg)> handler)
		{
			ConnectionVariant connection = typename MESSAGE_TYPE::GetSignal()->connect(handler);
			auto it = m_Connections.insert(m_Connections.cend(), connection);

			return (m_Connections.end() != it);
		}

	public:
		template<typename MESSAGE_TYPE>
		bool RegisterSlot_FilterByDeviceId(std::function<void(const MESSAGE_TYPE& msg)> handler, uint8_t device_id)
		{			
			ConnectionVariant connection = std::make_unique<FilteredSlot_ByDeviceId<MESSAGE_TYPE>>(handler, device_id);
			auto it = m_Connections.insert(m_Connections.cend(), std::move(connection));

			return (m_Connections.end() != it);
		}

	private:
		std::vector<ConnectionVariant> m_Connections;
	};

}
// namespace AqualinkAutomate::Utility
