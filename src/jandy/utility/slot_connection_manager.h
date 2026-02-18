#pragma once

#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <variant>
#include <vector>

#include <boost/signals2.hpp>

#include "devices/jandy_device_id.h"
#include "formatters/jandy_device_formatters.h"
#include "utility/filtered_slot.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

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
			LogDebug(Channel::Signals, std::format("Registering general handler for messages of type: {}", typeid(MESSAGE_TYPE*).name()));

			ConnectionVariant connection = MESSAGE_TYPE::GetSignal()->connect(handler);
			auto it = m_Connections.insert(m_Connections.cend(), connection);

			return (m_Connections.end() != it);
		}

	public:
		template<typename MESSAGE_TYPE>
		bool RegisterSlot_FilterByDeviceId(std::function<void(const MESSAGE_TYPE& msg)> handler, Devices::JandyDeviceId device_id)
		{
			LogDebug(Channel::Signals, std::format("Registering filtered handler for device id {} for messages of type: {}", device_id, typeid(MESSAGE_TYPE*).name()));

			ConnectionVariant connection = std::make_unique<FilteredSlot_ByDeviceId<MESSAGE_TYPE>>(handler, device_id);
			auto it = m_Connections.insert(m_Connections.cend(), std::move(connection));

			return (m_Connections.end() != it);
		}

	private:
		std::vector<ConnectionVariant> m_Connections;
	};

}
// namespace AqualinkAutomate::Utility
