#include <boost/uuid/uuid_generators.hpp>

#include "kernel/pump.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Kernel
{
	Pump::Pump(const std::string& label) :
		Pump(label, PumpStatus::Unknown)
	{
	}

	Pump::Pump(const std::string& label, const PumpStatus state) :
		AuxillaryBaseWithState<PumpStatus>(label, state)
	{
	}

	boost::uuids::uuid Pump::Id() const
	{
		static const boost::uuids::uuid this_id = boost::uuids::random_generator()();
		return this_id;
	}

	Pump& Pump::operator=(const Utility::AuxillaryState& aux_state)
	{
		if (!aux_state.State().has_value())
		{
			LogDebug(Channel::Equipment, "Cannot set Pump from auxillary state; Utility::AuxillaryState::State() was in an error state");
		}
		else
		{
			AuxillaryBaseWithState::m_State = ConvertToPumpStatus(aux_state.State().value());
		}

		return *this;
	}

	PumpStatus Pump::ConvertToPumpStatus(Kernel::AuxillaryStates aux_state)
	{
		switch (aux_state)
		{
		case AuxillaryStates::On:
			return PumpStatus::Running;

		case AuxillaryStates::Off:
			return PumpStatus::Off;

		case AuxillaryStates::Enabled:
			[[fallthrough]];
		case AuxillaryStates::Pending:
			[[fallthrough]];
		case AuxillaryStates::Unknown:
			[[fallthrough]];
		default:
			return PumpStatus::Unknown;
		}
	}

}
// namespace AqualinkAutomate::Kernel
