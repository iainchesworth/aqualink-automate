#include <boost/uuid/uuid_generators.hpp>

#include "kernel/auxillary_devices/heater.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Kernel
{
	Heater::Heater(const std::string& label) :
		Heater(label, HeaterStatuses::NotInstalled)
	{
	}

	Heater::Heater(const std::string& label, const HeaterStatuses status) :
		AuxillaryBaseWithStatus<HeaterStatuses>(label, status),
		m_Id(boost::uuids::random_generator()())
	{
		AuxillaryTraits.Set(AuxillaryTraitsTypes::AuxillaryTypeTrait{}, AuxillaryTraitsTypes::AuxillaryTypes::Heater);
	}

	boost::uuids::uuid Heater::Id() const
	{
		return m_Id;
	}

	void Heater::Status(const HeaterStatuses heater_status)
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
			Status(ConvertToHeaterStatuses(aux_state.State().value()));
		}

		return *this;
	}

	Heater& Heater::operator=(const HeaterStatuses heater_status)
	{
		Status(heater_status);
		return *this;
	}

	HeaterStatuses Heater::ConvertToHeaterStatuses(Kernel::AuxillaryStatuses aux_status)
	{
		switch (aux_status)
		{
		case AuxillaryStatuses::On:
			return HeaterStatuses::Heating;

		case AuxillaryStatuses::Off:
			return HeaterStatuses::Off;

		case AuxillaryStatuses::Enabled:
			return HeaterStatuses::Enabled;

		case AuxillaryStatuses::Pending:
			[[fallthrough]];
		case AuxillaryStatuses::Unknown:
			[[fallthrough]];
		default:
			return HeaterStatuses::Unknown;
		}
	}

}
// namespace AqualinkAutomate::Kernel
