#pragma once

#include <boost/uuid/uuid.hpp>
#include <nlohmann/json.hpp>

namespace AqualinkAutomate::Kernel
{

	enum class DataHub_EventTypes
	{
		Chemistry,
		Temperature
	};

	class DataHub_Event
	{
	public:
		DataHub_Event(DataHub_EventTypes event_type);

	public:
		DataHub_EventTypes Type() const;

	public:
		virtual boost::uuids::uuid Id() const = 0;
		virtual nlohmann::json ToJSON() const = 0;

	private:
		DataHub_EventTypes m_EventType;
	};

}
// namespace AqualinkAutomate::Kernel
