#pragma once

#include "kernel/auxillary_traits.h"
#include "kernel/flow_rate.h"
#include "kernel/temperature.h"

namespace AqualinkAutomate::Kernel::AuxillaryTraits
{

	//---------------------------------------------------------------------
	// COMMMON TRAITS
	//---------------------------------------------------------------------

	enum class AuxillaryTypes
	{
		Auxillary,
		Chlorinator,
		Heater,
		Pump
	};

	class AuxillaryTypeTrait : public ImmutableTraitType<AuxillaryTypes>
	{
	public:
		virtual TraitKey Name() const final { return std::string{"AuxillaryTypesTrait"}; }
	};

	class DutyCycleTrait : public MutableTraitType<uint8_t>
	{
	public:
		virtual TraitKey Name() const final { return std::string{"DutyCycleTrait"}; }
	};

	//---------------------------------------------------------------------
	// AUXILLARY TRAITS
	//---------------------------------------------------------------------



	//---------------------------------------------------------------------
	// CHLORINATOR TRAITS
	//---------------------------------------------------------------------


	

	//---------------------------------------------------------------------
	// HEATER TRAITS
	//---------------------------------------------------------------------

	class TargetTemperatureTrait : public MutableTraitType<Temperature>
	{
	public:
		virtual TraitKey Name() const final { return std::string{"TargetTemperatureTrait"}; }
	};

	//---------------------------------------------------------------------
	// PUMP TRAITS
	//---------------------------------------------------------------------

	class FlowRateTrait : public MutableTraitType<Kernel::FlowRate>
	{
	public:
		virtual TraitKey Name() const final { return std::string{"FlowRateTrait"}; }
	};

	//---------------------------------------------------------------------
	// .... TRAITS
	//---------------------------------------------------------------------



}
// namespace AqualinkAutomate::Kernel::AuxillaryTraits
