#pragma once

#include <set>
#include <string>

#include <boost/system/error_code.hpp>

#include "kernel/auxillary_devices/auxillary_status.h"
#include "kernel/auxillary_devices/chlorinator_boost_mode.h"
#include "kernel/auxillary_devices/chlorinator_status.h"
#include "kernel/auxillary_devices/heater_status.h"
#include "kernel/auxillary_devices/pump_speed.h"
#include "kernel/auxillary_devices/pump_status.h"
#include "kernel/auxillary_devices/pump_type.h"
#include "kernel/auxillary_traits/auxillary_traits_base.h"
#include "kernel/body_of_water_ids.h"
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
		Cleaner,
		Heater,
		Pump,
		Spillover,
		Sprinkler,
		Unknown
	};

	class AuxillaryTypeTrait : public ImmutableTraitType<const AuxillaryTypes>
	{
	public:
		TraitKey Name() const final { return std::string{"AuxillaryTypesTrait"}; }
	};

	class DutyCycleTrait : public MutableTraitType<uint8_t>
	{
	public:
		TraitKey Name() const final { return std::string{"DutyCycleTrait"}; }
	};

	class ErrorCodesTrait : public ImmutableTraitType<std::set<boost::system::error_code>>
	{
	public:
		TraitKey Name() const final { return std::string{"ErrorCodesTrait"}; }
	};

	class LabelTrait : public MutableTraitType<const std::string>
	{
	public:
		static const std::string COMMON_LABEL_FILTER_PUMP;

	public:
		TraitKey Name() const final { return std::string{"LabelTrait"}; }
	};

	class StatusTrait : public ImmutableTraitType<const std::string>
	{
	public:
		TraitKey Name() const final { return std::string{"StatusTrait_MultipleTraitTypes"}; }
	};

	//---------------------------------------------------------------------
	// AUXILLARY TRAITS
	//---------------------------------------------------------------------

	class AuxillaryStatusTrait : public MutableTraitType<AuxillaryStatuses>
	{
	public:
		TraitKey Name() const final { return std::string{"StatusTrait_MultipleTraitTypes"}; }
	};

	//---------------------------------------------------------------------
	// CHLORINATOR TRAITS
	//---------------------------------------------------------------------

	class ChlorinatorStatusTrait : public MutableTraitType<ChlorinatorStatuses>
	{
	public:
		TraitKey Name() const final { return std::string{"StatusTrait_MultipleTraitTypes"}; }
	};

	class GeneratingPercentageTrait : public MutableTraitType<uint8_t>
	{
	public:
		TraitKey Name() const final { return std::string{"GeneratingPercentageTrait"}; }
	};

	class BoostModeTrait : public MutableTraitType<ChlorinatorBoostModes>
	{
	public:
		TraitKey Name() const final { return std::string{"BoostModeTrait"}; }
	};

	class ChlorinatorHealthTrait : public MutableTraitType<ChlorinatorHealth>
	{
	public:
		TraitKey Name() const final { return std::string{"ChlorinatorHealthTrait"}; }
	};

	//---------------------------------------------------------------------
	// HEATER TRAITS
	//---------------------------------------------------------------------

	class HeaterStatusTrait : public MutableTraitType<HeaterStatuses>
	{
	public:
		TraitKey Name() const final { return std::string{"StatusTrait_MultipleTraitTypes"}; }
	};

	class TargetTemperatureTrait : public MutableTraitType<Temperature>
	{
	public:
		TraitKey Name() const final { return std::string{"TargetTemperatureTrait"}; }
	};

	//---------------------------------------------------------------------
	// LIGHT TRAITS
	//---------------------------------------------------------------------

	class BrightnessLevelTrait : public MutableTraitType<uint8_t>
	{
	public:
		TraitKey Name() const final { return std::string{"BrightnessLevelTrait"}; }
	};

	class ColourTrait : public MutableTraitType<uint8_t>
	{
	public:
		TraitKey Name() const final { return std::string{"ColourTrait"}; }
	};

	//---------------------------------------------------------------------
	// PUMP TRAITS
	//---------------------------------------------------------------------

	class PumpSpeedTrait : public MutableTraitType<PumpSpeeds>
	{
	public:
		TraitKey Name() const final { return std::string{ "PumpSpeedTrait" }; }
	};

	class PumpStatusTrait : public MutableTraitType<PumpStatuses>
	{
	public:
		TraitKey Name() const final { return std::string{ "StatusTrait_MultipleTraitTypes" }; }
	};

	class PumpTypeTrait : public MutableTraitType<PumpTypes>
	{
	public:
		TraitKey Name() const final { return std::string{ "PumpTypeTrait" }; }
	};

	class FlowRateTrait : public MutableTraitType<Kernel::FlowRate>
	{
	public:
		TraitKey Name() const final { return std::string{"FlowRateTrait"}; }
	};

	//---------------------------------------------------------------------
	// BODY-OF-WATER TRAITS
	//---------------------------------------------------------------------

	class BodyOfWaterTrait : public MutableTraitType<BodyOfWaterIds>
	{
	public:
		TraitKey Name() const final { return std::string{"BodyOfWaterTrait"}; }
	};

	//---------------------------------------------------------------------
	// .... TRAITS
	//---------------------------------------------------------------------



}
// namespace AqualinkAutomate::Kernel::AuxillaryTraitsTypes
