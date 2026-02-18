#pragma once

#include <memory>
#include <string>
#include <vector>

#include "kernel/auxillary_devices/auxillary_device.h"
#include "kernel/data_hub.h"
#include "kernel/pool_configurations.h"

namespace AqualinkAutomate::Test::Scenarios
{

	/// Helper to create a device with common traits set.
	std::shared_ptr<Kernel::AuxillaryDevice> MakeAuxillaryDevice(const std::string& label, Kernel::AuxillaryStatuses status);
	std::shared_ptr<Kernel::AuxillaryDevice> MakeChlorinatorDevice(const std::string& label, uint8_t percentage, uint16_t salt_ppm);
	std::shared_ptr<Kernel::AuxillaryDevice> MakeHeaterDevice(const std::string& label, Kernel::HeaterStatuses status, Kernel::BodyOfWaterIds body);
	std::shared_ptr<Kernel::AuxillaryDevice> MakePumpDevice(const std::string& label, Kernel::PumpStatuses status, Kernel::BodyOfWaterIds body);

	//---------------------------------------------------------------------
	// POOL ONLY (SingleBody)
	//---------------------------------------------------------------------

	/// Populate a DataHub with a pool-only (SingleBody) configuration.
	/// Creates: Filter Pump, Pool Heat, 2 auxiliaries, AquaPure chlorinator.
	/// Sets: pool temp, air temp, pool setpoint, salt level.
	void PopulatePoolOnly(Kernel::DataHub& hub);

	//---------------------------------------------------------------------
	// SPA ONLY (SingleBody)
	//---------------------------------------------------------------------

	/// Populate a DataHub with a spa-only (SingleBody) configuration.
	/// Creates: Filter Pump, Spa Heat, Aux1 (jets), Aux2 (spa light).
	/// Sets: spa temp, air temp, spa setpoint.
	void PopulateSpaOnly(Kernel::DataHub& hub);

	//---------------------------------------------------------------------
	// DUAL BODY, SHARED EQUIPMENT
	//---------------------------------------------------------------------

	/// Populate a DataHub with a dual-body shared-equipment configuration.
	/// Creates: Filter Pump, Pool Heat, Spa Heat, auxiliaries, chlorinator.
	/// Sets: pool temp, spa temp, air temp, both setpoints.
	void PopulateDualBodyShared(Kernel::DataHub& hub);

	//---------------------------------------------------------------------
	// DUAL BODY, DUAL EQUIPMENT
	//---------------------------------------------------------------------

	/// Populate a DataHub with a dual-body dual-equipment configuration.
	/// Creates: Pool Pump, Spa Pump, Pool Heat, Spa Heat, auxiliaries, chlorinator.
	/// Sets: pool temp, spa temp, air temp, both setpoints.
	void PopulateDualBodyDual(Kernel::DataHub& hub);

}
// namespace AqualinkAutomate::Test::Scenarios
