#include <cstdint>
#include <format>

#include <boost/uuid/uuid_generators.hpp>
#include <magic_enum.hpp>

#include "formatters/orp_formatter.h"
#include "formatters/ph_formatter.h"
#include "kernel/data_hub_event_chemistry.h"

namespace AqualinkAutomate::Kernel
{

	DataHub_Event_Chemistry::DataHub_Event_Chemistry() :
		DataHub_Event(DataHub_EventTypes::Chemistry),
		m_ORP(std::nullopt),
		m_pH(std::nullopt)
	{
	}

	std::optional<Kernel::ORP> DataHub_Event_Chemistry::ORP() const
	{
		return m_ORP;
	}

	std::optional<Kernel::pH> DataHub_Event_Chemistry::pH() const
	{
		return m_pH;
	}

	void DataHub_Event_Chemistry::ORP(Kernel::ORP orp)
	{
		m_ORP = orp;
	}

	void DataHub_Event_Chemistry::pH(Kernel::pH pH)
	{
		m_pH = pH;
	}

	boost::uuids::uuid DataHub_Event_Chemistry::Id() const
	{
		static boost::uuids::uuid id{ boost::uuids::random_generator()() };
		return id;
	}

	void to_json(nlohmann::json& j, const DataHub_Event_Chemistry& event)
	{
		if (event.ORP().has_value())
		{
			auto orp = event.ORP().value();
			j["orp"] = static_cast<uint16_t>(orp().value());
		}

		if (event.pH().has_value())
		{
			auto ph = event.pH().value();
			j["ph"] = static_cast<double>(ph());
		}
	}

}
// namespace AqualinkAutomate::Kernel
