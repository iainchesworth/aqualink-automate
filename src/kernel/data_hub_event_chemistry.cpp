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
		m_pH(std::nullopt),
		m_SaltLevel(std::nullopt)
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

	std::optional<Units::ppm_quantity> DataHub_Event_Chemistry::SaltLevel() const
	{
		return m_SaltLevel;
	}

	void DataHub_Event_Chemistry::ORP(const Kernel::ORP& orp)
	{
		m_ORP = orp;
	}

	void DataHub_Event_Chemistry::pH(const Kernel::pH& pH)
	{
		m_pH = pH;
	}

	void DataHub_Event_Chemistry::SaltLevel(const Units::ppm_quantity& salt_level_in_ppm)
	{
		m_SaltLevel = salt_level_in_ppm;
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

		if (event.SaltLevel().has_value())
		{
			auto salt_level = event.SaltLevel().value();
			j["salt_level"] = static_cast<uint16_t>(salt_level.value());
		}
	}

}
// namespace AqualinkAutomate::Kernel
