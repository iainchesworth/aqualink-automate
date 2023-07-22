#include <boost/uuid/uuid_generators.hpp>

#include "kernel/auxillary_devices/auxillary.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Kernel
{
	Auxillary::Auxillary(const std::string& label) :
		Auxillary(label, AuxillaryStatuses::Unknown)
	{
	}

	Auxillary::Auxillary(const std::string& label, const AuxillaryStatuses status) :
		AuxillaryBaseWithStatus(label, status),
		m_Id(boost::uuids::random_generator()())
	{
		AuxillaryTraits.Set(AuxillaryTraitsTypes::AuxillaryTypeTrait{}, AuxillaryTraitsTypes::AuxillaryTypes::Auxillary);
	}

	boost::uuids::uuid Auxillary::Id() const
	{
		return m_Id;
	}

	void Auxillary::Status(const AuxillaryStatuses aux_status)
	{
		AuxillaryBaseWithStatus::m_Status = aux_status;
	}

	Auxillary& Auxillary::operator=(const Utility::AuxillaryState& aux_state)
	{
		if (!aux_state.State().has_value())
		{
			LogDebug(Channel::Equipment, "Cannot set auxillary state; Utility::AuxillaryState::State() was in an error state");
		}
		else
		{
			AuxillaryBaseWithStatus::m_Status = aux_state.State().value();
		}

		return *this;
	}

}
// namespace AqualinkAutomate::Kernel
