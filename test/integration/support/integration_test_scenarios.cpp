#include "kernel/auxillary_devices/auxillary_device.h"
#include "kernel/auxillary_devices/auxillary_status.h"
#include "kernel/auxillary_devices/chlorinator_boost_mode.h"
#include "kernel/auxillary_devices/chlorinator_status.h"
#include "kernel/auxillary_devices/heater_status.h"
#include "kernel/auxillary_devices/pump_status.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "kernel/body_of_water_ids.h"
#include "kernel/data_hub.h"
#include "kernel/pool_configurations.h"
#include "kernel/temperature.h"
#include "types/units_dimensionless.h"

#include "support/integration_test_scenarios.h"

using namespace AqualinkAutomate::Kernel;
using namespace AqualinkAutomate::Kernel::AuxillaryTraitsTypes;
using namespace AqualinkAutomate::Units;

namespace AqualinkAutomate::Test::Scenarios
{

	std::shared_ptr<AuxillaryDevice> MakeAuxillaryDevice(const std::string& label, AuxillaryStatuses status)
	{
		auto device = std::make_shared<AuxillaryDevice>(label);
		device->AuxillaryTraits.Set(AuxillaryTypeTrait{}, AuxillaryTypes::Auxillary);
		device->AuxillaryTraits.Set(LabelTrait{}, label);
		device->AuxillaryTraits.Set(AuxillaryStatusTrait{}, status);
		return device;
	}

	std::shared_ptr<AuxillaryDevice> MakeChlorinatorDevice(const std::string& label, uint8_t percentage, uint16_t salt_ppm)
	{
		auto device = std::make_shared<AuxillaryDevice>(label);
		device->AuxillaryTraits.Set(AuxillaryTypeTrait{}, AuxillaryTypes::Chlorinator);
		device->AuxillaryTraits.Set(LabelTrait{}, label);
		device->AuxillaryTraits.Set(BodyOfWaterTrait{}, BodyOfWaterIds::Shared);
		device->AuxillaryTraits.Set(ChlorinatorStatusTrait{}, ChlorinatorStatuses::On);
		device->AuxillaryTraits.Set(GeneratingPercentageTrait{}, percentage);
		device->AuxillaryTraits.Set(BoostModeTrait{}, ChlorinatorBoostModes::Off);
		device->AuxillaryTraits.Set(ChlorinatorHealthTrait{}, ChlorinatorHealth::Ok);
		return device;
	}

	std::shared_ptr<AuxillaryDevice> MakeHeaterDevice(const std::string& label, HeaterStatuses status, BodyOfWaterIds body)
	{
		auto device = std::make_shared<AuxillaryDevice>(label);
		device->AuxillaryTraits.Set(AuxillaryTypeTrait{}, AuxillaryTypes::Heater);
		device->AuxillaryTraits.Set(LabelTrait{}, label);
		device->AuxillaryTraits.Set(HeaterStatusTrait{}, status);
		device->AuxillaryTraits.Set(BodyOfWaterTrait{}, body);
		return device;
	}

	std::shared_ptr<AuxillaryDevice> MakePumpDevice(const std::string& label, PumpStatuses status, BodyOfWaterIds body)
	{
		auto device = std::make_shared<AuxillaryDevice>(label);
		device->AuxillaryTraits.Set(AuxillaryTypeTrait{}, AuxillaryTypes::Pump);
		device->AuxillaryTraits.Set(LabelTrait{}, label);
		device->AuxillaryTraits.Set(PumpStatusTrait{}, status);
		device->AuxillaryTraits.Set(BodyOfWaterTrait{}, body);
		return device;
	}

	//---------------------------------------------------------------------
	// POOL ONLY
	//---------------------------------------------------------------------

	void PopulatePoolOnly(DataHub& hub)
	{
		hub.ApplyPoolConfiguration(PoolConfigurations::SingleBody);
		hub.SystemTemperatureUnits(TemperatureUnits::Fahrenheit);

		// Temperatures
		hub.AirTemp(Temperature::ConvertToTemperatureInFahrenheit(78.0));
		hub.PoolTemp(Temperature::ConvertToTemperatureInFahrenheit(82.0));
		hub.PoolTempSetpoint(Temperature::ConvertToTemperatureInFahrenheit(84.0));   // TEMP1 (priority)
		hub.PoolTempSetpoint2(Temperature::ConvertToTemperatureInFahrenheit(70.0));  // TEMP2 (maintenance, < TEMP1)
		hub.FreezeProtectPoint(Temperature::ConvertToTemperatureInFahrenheit(36.0));

		// Chemistry
		hub.SaltLevel(3200.0 * Units::ppm);

		// Devices
		hub.Devices.Add(MakePumpDevice("Filter Pump", PumpStatuses::Running, BodyOfWaterIds::Shared));
		hub.Devices.Add(MakeHeaterDevice("Pool Heat", HeaterStatuses::Enabled, BodyOfWaterIds::Pool));
		hub.Devices.Add(MakeAuxillaryDevice("Aux1", AuxillaryStatuses::Off));
		hub.Devices.Add(MakeAuxillaryDevice("Aux2", AuxillaryStatuses::On));
		hub.Devices.Add(MakeChlorinatorDevice("AquaPure", 50, 3200));
	}

	//---------------------------------------------------------------------
	// SPA ONLY
	//---------------------------------------------------------------------

	void PopulateSpaOnly(DataHub& hub)
	{
		hub.ApplyPoolConfiguration(PoolConfigurations::SingleBody);
		hub.CirculationMode = CirculationModes::Spa;
		hub.SystemTemperatureUnits(TemperatureUnits::Fahrenheit);

		// Temperatures
		hub.AirTemp(Temperature::ConvertToTemperatureInFahrenheit(72.0));
		hub.SpaTemp(Temperature::ConvertToTemperatureInFahrenheit(101.0));
		hub.SpaTempSetpoint(Temperature::ConvertToTemperatureInFahrenheit(102.0));

		// Devices
		hub.Devices.Add(MakePumpDevice("Filter Pump", PumpStatuses::Running, BodyOfWaterIds::Shared));
		hub.Devices.Add(MakeHeaterDevice("Spa Heat", HeaterStatuses::Heating, BodyOfWaterIds::Spa));
		hub.Devices.Add(MakeAuxillaryDevice("Aux1", AuxillaryStatuses::On));   // jets
		hub.Devices.Add(MakeAuxillaryDevice("Aux2", AuxillaryStatuses::Off));  // spa light
	}

	//---------------------------------------------------------------------
	// DUAL BODY, SHARED EQUIPMENT
	//---------------------------------------------------------------------

	void PopulateDualBodyShared(DataHub& hub)
	{
		hub.ApplyPoolConfiguration(PoolConfigurations::DualBody_SharedEquipment);
		hub.SystemTemperatureUnits(TemperatureUnits::Fahrenheit);

		// Temperatures
		hub.AirTemp(Temperature::ConvertToTemperatureInFahrenheit(76.0));
		hub.PoolTemp(Temperature::ConvertToTemperatureInFahrenheit(80.0));
		hub.SpaTemp(Temperature::ConvertToTemperatureInFahrenheit(100.0));
		hub.PoolTempSetpoint(Temperature::ConvertToTemperatureInFahrenheit(82.0));
		hub.SpaTempSetpoint(Temperature::ConvertToTemperatureInFahrenheit(102.0));
		hub.FreezeProtectPoint(Temperature::ConvertToTemperatureInFahrenheit(36.0));

		// Chemistry
		hub.SaltLevel(3400.0 * Units::ppm);

		// Devices - shared filter pump
		hub.Devices.Add(MakePumpDevice("Filter Pump", PumpStatuses::Running, BodyOfWaterIds::Shared));
		hub.Devices.Add(MakeHeaterDevice("Pool Heat", HeaterStatuses::Off, BodyOfWaterIds::Pool));
		hub.Devices.Add(MakeHeaterDevice("Spa Heat", HeaterStatuses::Enabled, BodyOfWaterIds::Spa));
		hub.Devices.Add(MakeAuxillaryDevice("Aux1", AuxillaryStatuses::Off));
		hub.Devices.Add(MakeAuxillaryDevice("Aux2", AuxillaryStatuses::Off));
		hub.Devices.Add(MakeAuxillaryDevice("Aux3", AuxillaryStatuses::On));
		hub.Devices.Add(MakeChlorinatorDevice("AquaPure", 60, 3400));
	}

	//---------------------------------------------------------------------
	// DUAL BODY, DUAL EQUIPMENT
	//---------------------------------------------------------------------

	void PopulateDualBodyDual(DataHub& hub)
	{
		hub.ApplyPoolConfiguration(PoolConfigurations::DualBody_DualEquipment);
		hub.SystemTemperatureUnits(TemperatureUnits::Fahrenheit);

		// Temperatures
		hub.AirTemp(Temperature::ConvertToTemperatureInFahrenheit(74.0));
		hub.PoolTemp(Temperature::ConvertToTemperatureInFahrenheit(79.0));
		hub.SpaTemp(Temperature::ConvertToTemperatureInFahrenheit(99.0));
		hub.PoolTempSetpoint(Temperature::ConvertToTemperatureInFahrenheit(81.0));
		hub.SpaTempSetpoint(Temperature::ConvertToTemperatureInFahrenheit(103.0));
		hub.FreezeProtectPoint(Temperature::ConvertToTemperatureInFahrenheit(35.0));

		// Chemistry
		hub.SaltLevel(3100.0 * Units::ppm);

		// Devices - separate pool and spa pumps
		hub.Devices.Add(MakePumpDevice("Pool Pump", PumpStatuses::Running, BodyOfWaterIds::Pool));
		hub.Devices.Add(MakePumpDevice("Spa Pump", PumpStatuses::Off, BodyOfWaterIds::Spa));
		hub.Devices.Add(MakeHeaterDevice("Pool Heat", HeaterStatuses::Heating, BodyOfWaterIds::Pool));
		hub.Devices.Add(MakeHeaterDevice("Spa Heat", HeaterStatuses::Off, BodyOfWaterIds::Spa));
		hub.Devices.Add(MakeAuxillaryDevice("Aux1", AuxillaryStatuses::On));
		hub.Devices.Add(MakeAuxillaryDevice("Aux2", AuxillaryStatuses::Off));
		hub.Devices.Add(MakeChlorinatorDevice("AquaPure", 70, 3100));
	}

}
// namespace AqualinkAutomate::Test::Scenarios
