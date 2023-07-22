#include <boost/uuid/uuid_generators.hpp>

#include "kernel/auxillary_devices/chlorinator.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Kernel
{

	Chlorinator::Chlorinator(const std::string& label) :
		Chlorinator(label, ChlorinatorStatuses::Unknown)
	{
	}

	Chlorinator::Chlorinator(const std::string& label, const ChlorinatorStatuses status) :
		AuxillaryBaseWithStatus<ChlorinatorStatuses>(label, status),
		m_Id(boost::uuids::random_generator()())
	{
		AuxillaryTraits.Set(AuxillaryTraitsTypes::AuxillaryTypeTrait{}, AuxillaryTraitsTypes::AuxillaryTypes::Chlorinator);
	}

	boost::uuids::uuid Chlorinator::Id() const
	{
		return m_Id;
	}

	void Chlorinator::Status(const ChlorinatorStatuses Chlorinator_status)
	{
		AuxillaryBaseWithStatus::m_Status = Chlorinator_status;
	}

	Chlorinator& Chlorinator::operator=(const Utility::AuxillaryState& aux_state)
	{
		if (!aux_state.State().has_value())
		{
			LogDebug(Channel::Equipment, "Cannot set Chlorinator from auxillary state; Utility::AuxillaryState::State() was in an error state");
		}
		else
		{
			Status(ConvertToChlorinatorStatuses(aux_state.State().value()));
		}

		return *this;
	}

	Chlorinator& Chlorinator::operator=(const ChlorinatorStatuses Chlorinator_status)
	{
		Status(Chlorinator_status);
		return *this;
	}

	ChlorinatorStatuses Chlorinator::ConvertToChlorinatorStatuses(Kernel::AuxillaryStatuses aux_status)
	{
		switch (aux_status)
		{
		case AuxillaryStatuses::On:
			return ChlorinatorStatuses::Running;

		case AuxillaryStatuses::Off:
			return ChlorinatorStatuses::Off;

		case AuxillaryStatuses::Enabled:
			[[fallthrough]];
		case AuxillaryStatuses::Pending:
			[[fallthrough]];
		case AuxillaryStatuses::Unknown:
			[[fallthrough]];
		default:
			return ChlorinatorStatuses::Unknown;
		}
	}

}
// namespace AqualinkAutomate::Kernel
