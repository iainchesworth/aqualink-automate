#include <boost/uuid/uuid_generators.hpp>

#include "kernel/auxillary_traits_types.h"
#include "kernel/chlorinator.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Kernel
{

	Chlorinator::Chlorinator(const std::string& label) :
		Chlorinator(label, ChlorinatorStatus::Unknown)
	{
	}

	Chlorinator::Chlorinator(const std::string& label, const ChlorinatorStatus status) :
		AuxillaryBaseWithStatus<ChlorinatorStatus>(label, status)
	{
		AuxillaryTraits.Set(AuxillaryTraits::AuxillaryTypeTrait{}, AuxillaryTraits::AuxillaryTypes::Chlorinator);
	}

	boost::uuids::uuid Chlorinator::Id() const
	{
		static const boost::uuids::uuid this_id = boost::uuids::random_generator()();
		return this_id;
	}

	void Chlorinator::Status(const ChlorinatorStatus Chlorinator_status)
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
			Status(ConvertToChlorinatorStatus(aux_state.State().value()));
		}

		return *this;
	}

	Chlorinator& Chlorinator::operator=(const ChlorinatorStatus Chlorinator_status)
	{
		Status(Chlorinator_status);
		return *this;
	}

	ChlorinatorStatus Chlorinator::ConvertToChlorinatorStatus(Kernel::AuxillaryStatuses aux_status)
	{
		switch (aux_status)
		{
		case AuxillaryStatuses::On:
			return ChlorinatorStatus::Running;

		case AuxillaryStatuses::Off:
			return ChlorinatorStatus::Off;

		case AuxillaryStatuses::Enabled:
			[[fallthrough]];
		case AuxillaryStatuses::Pending:
			[[fallthrough]];
		case AuxillaryStatuses::Unknown:
			[[fallthrough]];
		default:
			return ChlorinatorStatus::Unknown;
		}
	}

}
// namespace AqualinkAutomate::Kernel
