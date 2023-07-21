#include <boost/uuid/uuid_generators.hpp>

#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "kernel/pump.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Kernel
{

	Pump::Pump(const std::string& label) :
		Pump(label, PumpStatus::Unknown)
	{
	}

	Pump::Pump(const std::string& label, const PumpStatus status) :
		AuxillaryBaseWithStatus<PumpStatus>(label, status),
		m_Id(boost::uuids::random_generator()())
	{
		AuxillaryTraits.Set(AuxillaryTraitsTypes::AuxillaryTypeTrait{}, AuxillaryTraitsTypes::AuxillaryTypes::Pump);
	}

	boost::uuids::uuid Pump::Id() const
	{
		return m_Id;
	}

	void Pump::Status(const PumpStatus pump_status)
	{
		AuxillaryBaseWithStatus::m_Status = pump_status;
	}

	Pump& Pump::operator=(const Utility::AuxillaryState& aux_state)
	{
		if (!aux_state.State().has_value())
		{
			LogDebug(Channel::Equipment, "Cannot set Pump from auxillary state; Utility::AuxillaryState::State() was in an error state");
		}
		else
		{
			Status(ConvertToPumpStatus(aux_state.State().value()));
		}

		return *this;
	}

	Pump& Pump::operator=(const PumpStatus pump_status)
	{
		Status(pump_status);
		return *this;
	}

	PumpStatus Pump::ConvertToPumpStatus(Kernel::AuxillaryStatuses aux_status)
	{
		switch (aux_status)
		{
		case AuxillaryStatuses::On:
			return PumpStatus::Running;

		case AuxillaryStatuses::Off:
			return PumpStatus::Off;

		case AuxillaryStatuses::Enabled:
			[[fallthrough]];
		case AuxillaryStatuses::Pending:
			[[fallthrough]];
		case AuxillaryStatuses::Unknown:
			[[fallthrough]];
		default:
			return PumpStatus::Unknown;
		}
	}

}
// namespace AqualinkAutomate::Kernel
