#include <boost/algorithm/string.hpp>

#include "jandy/factories/jandy_auxillary_factory.h"
#include "jandy/utility/string_conversion/auxillary_state.h"
#include "kernel/auxillary_devices/chlorinator_status.h"
#include "kernel/auxillary_devices/heater_status.h"
#include "kernel/auxillary_devices/pump_status.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Factory
{

	JandyAuxillaryFactory::JandyAuxillaryFactory()
	{
	}

	JandyAuxillaryFactory& JandyAuxillaryFactory::Instance()
	{
		static JandyAuxillaryFactory instance;
		return instance;
	}

	std::shared_ptr<Kernel::AuxillaryDevice> JandyAuxillaryFactory::CreateDevice(const Utility::AuxillaryState& aux_state)
	{
		std::shared_ptr<Kernel::AuxillaryDevice> ptr(nullptr);

		if ((!aux_state.Label().has_value()) || (!aux_state.State().has_value()))
		{
			LogDebug(Channel::Equipment, "Received an invalid auxillary status; factory cannot create a new device using auxillary status");
		}
		else if (ptr = std::make_shared<Kernel::AuxillaryDevice>(); nullptr == ptr)
		{
			LogDebug(Channel::Equipment, "Factory cannot create a new device (result was nullptr)");
		}
		else
		{
			ptr->AuxillaryTraits.Set(AuxillaryTraitsTypes::LabelTrait{}, aux_state.Label().value());

			if (IsAuxillaryDevice(aux_state.Label().value()))
			{
				ptr->AuxillaryTraits.Set(AuxillaryTraitsTypes::AuxillaryTypeTrait{}, AuxillaryTraitsTypes::AuxillaryTypes::Auxillary);
				
				if (aux_state.State().has_value())
				{
					ptr->AuxillaryTraits.Set(AuxillaryTraitsTypes::AuxillaryStatusTrait{}, aux_state.State().value());
				}
			}
			else if (IsChlorinatorDevice(aux_state.Label().value()))
			{
				ptr->AuxillaryTraits.Set(AuxillaryTraitsTypes::AuxillaryTypeTrait{}, AuxillaryTraitsTypes::AuxillaryTypes::Chlorinator);

				if (aux_state.State().has_value())
				{
					ptr->AuxillaryTraits.Set(AuxillaryTraitsTypes::ChlorinatorStatusTrait{}, Kernel::ConvertToChlorinatorStatus(aux_state.State().value()));
				}
			}
			else if (IsCleanerDevice(aux_state.Label().value()))
			{
				ptr->AuxillaryTraits.Set(AuxillaryTraitsTypes::AuxillaryTypeTrait{}, AuxillaryTraitsTypes::AuxillaryTypes::Cleaner);
			}
			else if (IsHeaterDevice(aux_state.Label().value()))
			{
				// Pool Head, Spa Heat, Heat Pump
				//
				// Note that ordering means that "Heat Pump" is caught here!

				ptr->AuxillaryTraits.Set(AuxillaryTraitsTypes::AuxillaryTypeTrait{}, AuxillaryTraitsTypes::AuxillaryTypes::Heater);

				if (aux_state.State().has_value())
				{
					ptr->AuxillaryTraits.Set(AuxillaryTraitsTypes::HeaterStatusTrait{}, Kernel::ConvertToHeaterStatus(aux_state.State().value()));
				}
			}
			else if (IsPumpDevice(aux_state.Label().value()))
			{
				// Filter Pump, Pool Pump, Spa Pump
				//
				// Note that ordering means that "Heat Pump" is caught above!

				ptr->AuxillaryTraits.Set(AuxillaryTraitsTypes::AuxillaryTypeTrait{}, AuxillaryTraitsTypes::AuxillaryTypes::Pump);

				if (aux_state.State().has_value())
				{
					ptr->AuxillaryTraits.Set(AuxillaryTraitsTypes::PumpStatusTrait{}, Kernel::ConvertToPumpStatus(aux_state.State().value()));
				}
			}
			else if (IsSpilloverDevice(aux_state.Label().value()))
			{
				ptr->AuxillaryTraits.Set(AuxillaryTraitsTypes::AuxillaryTypeTrait{}, AuxillaryTraitsTypes::AuxillaryTypes::Spillover);
			}
			else if (IsSprinklerDevice(aux_state.Label().value()))
			{
				ptr->AuxillaryTraits.Set(AuxillaryTraitsTypes::AuxillaryTypeTrait{}, AuxillaryTraitsTypes::AuxillaryTypes::Sprinkler);
			}
			else
			{
				// Unknown device type...ignore it.
			}
		}

		return ptr;
	}

	bool JandyAuxillaryFactory::IsAuxillaryDevice(const std::string& label) const
	{
		static const std::string AUX_PREFIX { "Aux" };
		static const std::string EXTRA_AUX { "Extra Aux" };

		if ((label.starts_with(AUX_PREFIX)) || (EXTRA_AUX == label))
		{
			return true;
		}

		return false;
	}

	bool JandyAuxillaryFactory::IsChlorinatorDevice(const std::string& label) const
	{
		static const std::string CHLORINATOR { "Chlorinator" };
		return (CHLORINATOR == label);
	}

	bool JandyAuxillaryFactory::IsCleanerDevice(const std::string& label) const
	{
		static const std::string CLEANER { "Cleaner" };
		return (CLEANER == label);
	}

	bool JandyAuxillaryFactory::IsHeaterDevice(const std::string& label) const
	{
		return boost::algorithm::contains(label, "Heat");
	}

	bool JandyAuxillaryFactory::IsPumpDevice(const std::string& label) const
	{
		return boost::algorithm::contains(label, "Pump");
	}

	bool JandyAuxillaryFactory::IsSpilloverDevice(const std::string& label) const
	{
		static const std::string SPILLOVER { "Spillover" };
		return (SPILLOVER == label);
	}

	bool JandyAuxillaryFactory::IsSprinklerDevice(const std::string& label) const
	{
		return false;
	}

}
// namespace AqualinkAutomate::Factory
