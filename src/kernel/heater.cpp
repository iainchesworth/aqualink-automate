#include <boost/uuid/uuid_generators.hpp>

#include "kernel/auxillary_traits_types.h"
#include "kernel/heater.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Kernel
{
	Heater::Heater(const std::string& label) :
		Heater(label, HeaterStatus::NotInstalled)
	{
	}

	Heater::Heater(const std::string& label, const HeaterStatus status) :
		AuxillaryBaseWithStatus<HeaterStatus>(label, status)
	{
		m_Traits.Set(AuxillaryTraits::AuxillaryTypeTrait{}, AuxillaryTraits::AuxillaryTypes::Heater);
	}

	boost::uuids::uuid Heater::Id() const
	{
		static const boost::uuids::uuid this_id = boost::uuids::random_generator()();
		return this_id;
	}

	void Heater::Status(const HeaterStatus heater_status)
	{
		AuxillaryBaseWithStatus::m_Status = heater_status;
	}

	Heater& Heater::operator=(const Utility::AuxillaryState& aux_state)
	{
		if (!aux_state.State().has_value())
		{
			LogDebug(Channel::Equipment, "Cannot set Heater from auxillary state; Utility::AuxillaryState::State() was in an error state");
		}
		else
		{
			Status(ConvertToHeaterStatus(aux_state.State().value()));
		}

		return *this;
	}

	Heater& Heater::operator=(const HeaterStatus heater_status)
	{
		Status(heater_status);
		return *this;
	}

	HeaterStatus Heater::ConvertToHeaterStatus(Kernel::AuxillaryStatuses aux_status)
	{
		switch (aux_status)
		{
		case AuxillaryStatuses::On:
			return HeaterStatus::Heating;

		case AuxillaryStatuses::Off:
			return HeaterStatus::Off;

		case AuxillaryStatuses::Enabled:
			return HeaterStatus::Enabled;

		case AuxillaryStatuses::Pending:
			[[fallthrough]];
		case AuxillaryStatuses::Unknown:
			[[fallthrough]];
		default:
			return HeaterStatus::Unknown;
		}
	}

}
// namespace AqualinkAutomate::Kernel
