#include <boost/uuid/uuid_generators.hpp>

#include "kernel/auxillary.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Kernel
{
	Auxillary::Auxillary(const std::string& label) :
		Auxillary(label, AuxillaryStates::Unknown)
	{
	}

	Auxillary::Auxillary(const std::string& label, const AuxillaryStates state) :
		AuxillaryBaseWithState(label, state)
	{
	}

	boost::uuids::uuid Auxillary::Id() const
	{
		static const boost::uuids::uuid this_id = boost::uuids::random_generator()();
		return this_id;
	}

	Auxillary& Auxillary::operator=(const Utility::AuxillaryState& aux_state)
	{
		if (!aux_state.State().has_value())
		{
			LogDebug(Channel::Equipment, "Cannot set auxillary state; Utility::AuxillaryState::State() was in an error state");
		}
		else
		{
			AuxillaryBaseWithState::m_State = aux_state.State().value();
		}

		return *this;
	}

}
// namespace AqualinkAutomate::Kernel
