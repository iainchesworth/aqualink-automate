#pragma once

#include <set>
#include <string>

#include <boost/system/error_code.hpp>

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
		virtual TraitKey Name() const final { return std::string{"LabelTrait"}; }
	};

	class LabelTrait : public ImmutableTraitType<const std::string>
	{
	public:
		virtual TraitKey Name() const final { return std::string{"LabelTrait"}; }
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
