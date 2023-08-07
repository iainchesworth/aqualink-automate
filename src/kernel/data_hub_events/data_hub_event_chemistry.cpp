#include <cstdint>
#include <format>

#include <boost/uuid/uuid_generators.hpp>
#include <magic_enum.hpp>

#include "formatters/orp_formatter.h"
#include "formatters/ph_formatter.h"
#include "kernel/data_hub_events/data_hub_event_chemistry.h"

namespace AqualinkAutomate::Kernel
{

	DataHub_Event_Chemistry::DataHub_Event_Chemistry() :
		DataHub_Event(DataHub_EventTypes::Chemistry),
		m_ORP(std::nullopt),
		m_pH(std::nullopt),
		m_SaltLevel(std::nullopt)
	{
	}

	DataHub_Event_Chemistry::~DataHub_Event_Chemistry()
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

	nlohmann::json DataHub_Event_Chemistry::ToJSON() const
	{
		nlohmann::json j;

		if (m_ORP.has_value())
		{
			auto orp = m_ORP.value();
			j["orp"] = static_cast<uint16_t>(orp().value());
		}

		if (m_pH.has_value())
		{
			auto ph = m_pH.value();
			j["ph"] = static_cast<double>(ph());
		}

		if (m_SaltLevel.has_value())
		{
			auto salt_level = m_SaltLevel.value();
			j["salt_level"] = static_cast<uint16_t>(salt_level.value());
		}

		return j;
	}

}
// namespace AqualinkAutomate::Kernel
