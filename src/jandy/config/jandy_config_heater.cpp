#include <boost/uuid/uuid_generators.hpp>

#include "jandy/config/jandy_config_heater.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Config
{
	Heater::Heater(const std::string& label) :
		Heater(label, HeaterStatus::NotInstalled)
	{
	}

	Heater::Heater(const std::string& label, const HeaterStatus state) :
		AuxillaryBaseWithState<HeaterStatus>(label, state)
	{
	}

	boost::uuids::uuid Heater::Id() const
	{
		static const boost::uuids::uuid this_id = boost::uuids::random_generator()();
		return this_id;
	}

	Heater& Heater::operator=(const Utility::AuxillaryState& aux_state)
	{
		if (!aux_state.State().has_value())
		{
			LogDebug(Channel::Equipment, "Cannot set Heater from auxillary state; Utility::AuxillaryState::State() was in an error state");
		}
		else
		{
			AuxillaryBaseWithState::m_State = ConvertToHeaterStatus(aux_state.State().value());
		}

		return *this;
	}

	HeaterStatus Heater::ConvertToHeaterStatus(Config::AuxillaryStates aux_state)
	{
		switch (aux_state)
		{
		case AuxillaryStates::On:
			return HeaterStatus::Heating;

		case AuxillaryStates::Off:
			return HeaterStatus::Off;

		case AuxillaryStates::Enabled:
			return HeaterStatus::Enabled;

		case AuxillaryStates::Pending:
			[[fallthrough]];
		case AuxillaryStates::Unknown:
			[[fallthrough]];
		default:
			return HeaterStatus::Unknown;
		}
	}

}
// namespace AqualinkAutomate::Config
