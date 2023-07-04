#include "kernel/auxillary.h"
#include "kernel/heater.h"
#include "kernel/pump.h"
#include "jandy/factories/jandy_auxillary_factory.h"
#include "jandy/utility/string_conversion/auxillary_state.h"
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

	std::shared_ptr<Kernel::AuxillaryBase> JandyAuxillaryFactory::CreateDevice(const Utility::AuxillaryState& aux_state)
	{
		if ((!aux_state.Label().has_value()) || (!aux_state.State().has_value()))
		{
			LogDebug(Channel::Equipment, "Received an invalid auxillary status; factory cannot create a new device using auxillary status");
		}
		else if (IsAuxillaryDevice(aux_state.Label().value()))
		{
			auto ptr = std::make_shared<Kernel::Auxillary>(aux_state.Label().value());
			*ptr = aux_state;
			return ptr;
		}
		else if (IsCleanerDevice(aux_state.Label().value()))
		{
			auto ptr = std::make_shared<Kernel::Auxillary>(aux_state.Label().value());
			*ptr = aux_state;
			return ptr;
		}
		else if (IsHeaterDevice(aux_state.Label().value()))
		{
			// Pool Head, Spa Heat, Heat Pump
			//
			// Note that ordering means that "Heat Pump" is caught here!

			auto ptr = std::make_shared<Kernel::Heater>(aux_state.Label().value());
			*ptr = aux_state;
			return ptr;
		}
		else if (IsPumpDevice(aux_state.Label().value()))
		{
			// Filter Pump, Pool Pump, Spa Pump
			//
			// Note that ordering means that "Heat Pump" is caught above!

			auto ptr = std::make_shared<Kernel::Pump>(aux_state.Label().value());
			*ptr = aux_state;
			return ptr;
		}
		else if (IsSpilloverDevice(aux_state.Label().value()))
		{
			auto ptr = std::make_shared<Kernel::Auxillary>(aux_state.Label().value());
			*ptr = aux_state;
			return ptr;
		}
		else if (IsSprinklerDevice(aux_state.Label().value()))
		{
			///FIXME
		}
		else
		{
			///FIXME
		}

		return nullptr;
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

	bool JandyAuxillaryFactory::IsCleanerDevice(const std::string& label) const
	{
		static const std::string CLEANER { "Cleaner" };
		return (CLEANER == label);
	}

	bool JandyAuxillaryFactory::IsHeaterDevice(const std::string& label) const
	{
		return label.contains("Heat");
	}

	bool JandyAuxillaryFactory::IsPumpDevice(const std::string& label) const
	{
		return label.contains("Pump");
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
