#pragma once

#include <optional>

#include <boost/uuid/uuid.hpp>
#include <nlohmann/json.hpp>

#include "kernel/orp.h"
#include "kernel/ph.h"
#include "kernel/hub_events/data_hub_config_event.h"
#include "types/units_dimensionless.h"

namespace AqualinkAutomate::Kernel
{

	class DataHub_ConfigEvent_Chemistry : public DataHub_ConfigEvent
	{
	public:
		DataHub_ConfigEvent_Chemistry();
		virtual ~DataHub_ConfigEvent_Chemistry();

	public:
		std::optional<Kernel::ORP> ORP() const;
		std::optional<Kernel::pH> pH() const;
		std::optional<Units::ppm_quantity> SaltLevel() const;

	public:
		void ORP(const Kernel::ORP& orp);
		void pH(const Kernel::pH& pH);
		void SaltLevel(const Units::ppm_quantity& salt_level_in_ppm);

	public:
		virtual boost::uuids::uuid Id() const override;
		virtual nlohmann::json ToJSON() const override;

	private:
		std::optional<Kernel::ORP> m_ORP;
		std::optional<Kernel::pH> m_pH;
		std::optional<Units::ppm_quantity> m_SaltLevel;
	};

}
// namespace namespace AqualinkAutomate::Kernel
