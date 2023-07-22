#pragma once

#include <set>
#include <string>

#include <boost/system/error_code.hpp>

#include "kernel/auxillary_devices/auxillary_states.h"
#include "kernel/auxillary_devices/chlorinator_status.h"
#include "kernel/auxillary_devices/heater_status.h"
#include "kernel/auxillary_devices/pump_status.h"
#include "kernel/auxillary_traits/auxillary_traits_base.h"
#include "kernel/flow_rate.h"
#include "kernel/temperature.h"

namespace AqualinkAutomate::Kernel::AuxillaryTraitsTypes
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

	class AuxillaryTypeTrait : public ImmutableTraitType<const AuxillaryTypes>
	{
	public:
		virtual TraitKey Name() const final { return std::string{"AuxillaryTypesTrait"}; }
	};

	class DutyCycleTrait : public MutableTraitType<uint8_t>
	{
	public:
		virtual TraitKey Name() const final { return std::string{"DutyCycleTrait"}; }
	};

	class ErrorCodesTrait : public ImmutableTraitType<std::set<boost::system::error_code>>
	{
	public:
		virtual TraitKey Name() const final { return std::string{"ErrorCodesTrait"}; }
	};

	class LabelTrait : public ImmutableTraitType<const std::string>
	{
	public:
		virtual TraitKey Name() const final { return std::string{"LabelTrait"}; }
	};

	class StatusTrait : public ImmutableTraitType<const std::string>
	{
	public:
		virtual TraitKey Name() const final { return std::string{"StatusTrait_MultipleTraitTypes"}; }
	};

	//---------------------------------------------------------------------
	// AUXILLARY TRAITS
	//---------------------------------------------------------------------

	class AuxillaryStatusTrait : public MutableTraitType<AuxillaryStatuses>
	{
	public:
		virtual TraitKey Name() const final { return std::string{"StatusTrait_MultipleTraitTypes"}; }
	};

	//---------------------------------------------------------------------
	// CHLORINATOR TRAITS
	//---------------------------------------------------------------------

	class ChlorinatorStatusTrait : public MutableTraitType<ChlorinatorStatuses>
	{
	public:
		virtual TraitKey Name() const final { return std::string{"StatusTrait_MultipleTraitTypes"}; }
	};

	//---------------------------------------------------------------------
	// HEATER TRAITS
	//---------------------------------------------------------------------

	class HeaterStatusTrait : public MutableTraitType<HeaterStatuses>
	{
	public:
		virtual TraitKey Name() const final { return std::string{"StatusTrait_MultipleTraitTypes"}; }
	};

	class TargetTemperatureTrait : public MutableTraitType<Temperature>
	{
	public:
		virtual TraitKey Name() const final { return std::string{"TargetTemperatureTrait"}; }
	};

	//---------------------------------------------------------------------
	// LIGHT TRAITS
	//---------------------------------------------------------------------

	class BrightnessLevelTrait : public MutableTraitType<uint8_t>
	{
	public:
		virtual TraitKey Name() const final { return std::string{"BrightnessLevelTrait"}; }
	};

	class ColourTrait : public MutableTraitType<uint8_t>
	{
	public:
		virtual TraitKey Name() const final { return std::string{"ColourTrait"}; }
	};

	//---------------------------------------------------------------------
	// PUMP TRAITS
	//---------------------------------------------------------------------

	class PumpStatusTrait : public MutableTraitType<PumpStatuses>
	{
	public:
		virtual TraitKey Name() const final { return std::string{"StatusTrait_MultipleTraitTypes"}; }
	};

	class FlowRateTrait : public MutableTraitType<Kernel::FlowRate>
	{
	public:
		virtual TraitKey Name() const final { return std::string{"FlowRateTrait"}; }
	};

	//---------------------------------------------------------------------
	// .... TRAITS
	//---------------------------------------------------------------------



}
// namespace AqualinkAutomate::Kernel::AuxillaryTraitsTypes
