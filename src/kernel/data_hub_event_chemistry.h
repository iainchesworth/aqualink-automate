#pragma once

#include <optional>

#include <boost/uuid/uuid.hpp>
#include <nlohmann/json.hpp>

#include "kernel/data_hub_event.h"
#include "kernel/orp.h"
#include "kernel/ph.h"

namespace AqualinkAutomate::Kernel
{

	class DataHub_Event_Chemistry : public DataHub_Event
	{
	public:
		DataHub_Event_Chemistry();

	public:
		std::optional<Kernel::ORP> ORP() const;
		std::optional<Kernel::pH> pH() const;

	public:
		void ORP(Kernel::ORP orp);
		void pH(Kernel::pH pH);

	public:
		virtual boost::uuids::uuid Id() const override;

	private:
		std::optional<Kernel::ORP> m_ORP;
		std::optional<Kernel::pH> m_pH;
	};

	// Support the translation of the various WS event object types to JSON
	//
	// See https://github.com/nlohmann/json#arbitrary-types-conversions.

	void to_json(nlohmann::json& j, const DataHub_Event_Chemistry& event);

}
// namespace namespace AqualinkAutomate::Kernel
