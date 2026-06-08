#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include <boost/test/unit_test.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include "kernel/auxillary_devices/auxillary_device.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "kernel/data_hub.h"
#include "kernel/device_graph/device_graph.h"
#include "kernel/hub_events/data_hub_config_event.h"
#include "kernel/orp.h"
#include "kernel/ph.h"
#include "kernel/temperature.h"

using namespace AqualinkAutomate::Kernel;
using AqualinkAutomate::Kernel::AuxillaryTraitsTypes::AuxillaryTypeTrait;
using AqualinkAutomate::Kernel::AuxillaryTraitsTypes::AuxillaryTypes;
using AqualinkAutomate::Kernel::AuxillaryTraitsTypes::PumpTypeTrait;

namespace
{
	// Builds an AuxillaryDevice with a label + auxillary-type trait so it is
	// discoverable by the by-label and by-trait filtered views.
	std::shared_ptr<AuxillaryDevice> MakeTypedDevice(std::string_view label, AuxillaryTypes type)
	{
		auto device = std::make_shared<AuxillaryDevice>(label);
		device->AuxillaryTraits.Set(AuxillaryTypeTrait{}, type);
		return device;
	}
}

BOOST_AUTO_TEST_SUITE(DeviceGraph_TestSuite)

// =============================================================================
// CountById / FindById (routed through the shared filtered-view helpers)
// =============================================================================

BOOST_AUTO_TEST_CASE(FindById_KnownDevice_ReturnsDevice)
{
	DevicesGraph graph;

	auto device = MakeTypedDevice("Pool Pump", AuxillaryTypes::Pump);
	const auto id = device->Id();
	graph.Add(device);

	BOOST_CHECK_EQUAL(1u, graph.CountById(id));

	auto found = graph.FindById(id);
	BOOST_REQUIRE(nullptr != found);
	BOOST_CHECK(found->Id() == id);
}

BOOST_AUTO_TEST_CASE(FindById_UnknownDevice_ReturnsNullAndZeroCount)
{
	DevicesGraph graph;
	graph.Add(MakeTypedDevice("Pool Pump", AuxillaryTypes::Pump));

	const auto missing_id = boost::uuids::random_generator()();

	BOOST_CHECK_EQUAL(0u, graph.CountById(missing_id));
	BOOST_CHECK(nullptr == graph.FindById(missing_id));
}

// =============================================================================
// CountByLabel / HasAnyByLabel / FindByLabel
// =============================================================================

BOOST_AUTO_TEST_CASE(ByLabel_PresentAndAbsent)
{
	DevicesGraph graph;
	graph.Add(MakeTypedDevice("Spa Pump", AuxillaryTypes::Pump));

	BOOST_CHECK_EQUAL(1u, graph.CountByLabel("Spa Pump"));
	BOOST_CHECK(graph.HasAnyByLabel("Spa Pump"));
	BOOST_CHECK_EQUAL(1u, graph.FindByLabel("Spa Pump").size());

	BOOST_CHECK_EQUAL(0u, graph.CountByLabel("Nonexistent"));
	BOOST_CHECK(!graph.HasAnyByLabel("Nonexistent"));
	BOOST_CHECK(graph.FindByLabel("Nonexistent").empty());
}

BOOST_AUTO_TEST_CASE(FindByLabel_NoNullVerticesLeak)
{
	// The root vertex holds a null device pointer. The filtered view must never
	// surface it (the shared collect helper applies the null guard uniformly).
	DevicesGraph graph;
	graph.Add(MakeTypedDevice("Aux1", AuxillaryTypes::Auxillary));

	for (const auto& device : graph.FindByLabel("Aux1"))
	{
		BOOST_CHECK(nullptr != device);
	}
}

// =============================================================================
// CountByTrait / HasAnyByTrait / FindByTrait
// =============================================================================

BOOST_AUTO_TEST_CASE(ByTrait_CountAndExists_MatchFind)
{
	DevicesGraph graph;
	graph.Add(MakeTypedDevice("Pool Pump", AuxillaryTypes::Pump));
	graph.Add(MakeTypedDevice("Spa Pump", AuxillaryTypes::Pump));
	graph.Add(MakeTypedDevice("Pool Light", AuxillaryTypes::Light));

	BOOST_CHECK_EQUAL(2u, graph.CountByTrait(AuxillaryTypeTrait{}, AuxillaryTypes::Pump));
	BOOST_CHECK(graph.HasAnyByTrait(AuxillaryTypeTrait{}, AuxillaryTypes::Pump));
	BOOST_CHECK_EQUAL(2u, graph.FindByTrait(AuxillaryTypeTrait{}, AuxillaryTypes::Pump).size());

	BOOST_CHECK_EQUAL(1u, graph.CountByTrait(AuxillaryTypeTrait{}, AuxillaryTypes::Light));

	// No chlorinators were added.
	BOOST_CHECK_EQUAL(0u, graph.CountByTrait(AuxillaryTypeTrait{}, AuxillaryTypes::Chlorinator));
	BOOST_CHECK(!graph.HasAnyByTrait(AuxillaryTypeTrait{}, AuxillaryTypes::Chlorinator));
}

BOOST_AUTO_TEST_CASE(HasAnyByTrait_PresenceOnly)
{
	DevicesGraph graph;

	BOOST_CHECK(!graph.HasAnyByTrait(AuxillaryTypeTrait{}, AuxillaryTypes::Heater));

	graph.Add(MakeTypedDevice("Pool Heat", AuxillaryTypes::Heater));

	BOOST_CHECK(graph.HasAnyByTrait(AuxillaryTypeTrait{}, AuxillaryTypes::Heater));
	BOOST_CHECK(graph.HasAnyByTrait(AuxillaryTypeTrait{}));
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================
// DataHub device-type accessors + count/exists predicates
// =============================================================================

BOOST_AUTO_TEST_SUITE(DataHub_DeviceQueries_TestSuite)

BOOST_AUTO_TEST_CASE(DevicesOfType_MatchesTypedAccessors)
{
	DataHub hub;
	hub.Devices.Add(MakeTypedDevice("Pool Pump", AuxillaryTypes::Pump));
	hub.Devices.Add(MakeTypedDevice("Spa Pump", AuxillaryTypes::Pump));
	hub.Devices.Add(MakeTypedDevice("Pool Heat", AuxillaryTypes::Heater));
	hub.Devices.Add(MakeTypedDevice("Aux1", AuxillaryTypes::Auxillary));

	BOOST_CHECK_EQUAL(2u, hub.DevicesOfType(AuxillaryTypes::Pump).size());
	BOOST_CHECK_EQUAL(hub.Pumps().size(), hub.DevicesOfType(AuxillaryTypes::Pump).size());
	BOOST_CHECK_EQUAL(hub.Heaters().size(), hub.DevicesOfType(AuxillaryTypes::Heater).size());
	BOOST_CHECK_EQUAL(hub.Auxillaries().size(), hub.DevicesOfType(AuxillaryTypes::Auxillary).size());

	// Count / exists predicates agree with the materialised vectors.
	BOOST_CHECK_EQUAL(2u, hub.CountOfType(AuxillaryTypes::Pump));
	BOOST_CHECK(hub.HasAnyOfType(AuxillaryTypes::Pump));
	BOOST_CHECK_EQUAL(0u, hub.CountOfType(AuxillaryTypes::Chlorinator));
	BOOST_CHECK(!hub.HasAnyOfType(AuxillaryTypes::Chlorinator));
}

BOOST_AUTO_TEST_CASE(FilterPumps_CountAndExists)
{
	DataHub hub;

	BOOST_CHECK_EQUAL(0u, hub.CountFilterPumps());
	BOOST_CHECK(!hub.HasAnyFilterPumps());

	auto filter_pump = MakeTypedDevice("Filter Pump", AuxillaryTypes::Pump);
	filter_pump->AuxillaryTraits.Set(PumpTypeTrait{}, PumpTypes::FilterCirculation);
	hub.Devices.Add(filter_pump);

	BOOST_CHECK_EQUAL(1u, hub.CountFilterPumps());
	BOOST_CHECK(hub.HasAnyFilterPumps());
	BOOST_CHECK_EQUAL(static_cast<uint32_t>(hub.FilterPumps().size()), hub.CountFilterPumps());
}

// =============================================================================
// Config-update event emit helpers
// =============================================================================

BOOST_AUTO_TEST_CASE(TemperatureSetters_EmitConfigUpdateSignal)
{
	DataHub hub;

	int events = 0;
	auto connection = hub.ConfigUpdateSignal.connect(
		[&events](std::shared_ptr<DataHub_ConfigEvent>) { ++events; });

	hub.AirTemp(Temperature::ConvertToTemperatureInCelsius(25.0));
	hub.PoolTemp(Temperature::ConvertToTemperatureInCelsius(26.0));
	hub.SpaTemp(Temperature::ConvertToTemperatureInCelsius(27.0));

	BOOST_CHECK_EQUAL(3, events);

	connection.disconnect();
}

BOOST_AUTO_TEST_CASE(ChemistrySetters_EmitConfigUpdateSignal)
{
	DataHub hub;

	int events = 0;
	auto connection = hub.ConfigUpdateSignal.connect(
		[&events](std::shared_ptr<DataHub_ConfigEvent>) { ++events; });

	hub.ORP(ORP{ 700.0 });
	hub.pH(pH{ 7.4f });

	BOOST_CHECK_EQUAL(2, events);

	connection.disconnect();
}

BOOST_AUTO_TEST_CASE(FreezeProtectAndUnits_DeliberatelyDoNotEmitSignal)
{
	DataHub hub;

	int events = 0;
	auto connection = hub.ConfigUpdateSignal.connect(
		[&events](std::shared_ptr<DataHub_ConfigEvent>) { ++events; });

	// These setters intentionally do not emit a ConfigUpdateSignal: the
	// temperature event carries no field for them, so an emitted event would be
	// an empty no-op for consumers.
	hub.FreezeProtectPoint(Temperature::ConvertToTemperatureInCelsius(4.0));
	hub.SystemTemperatureUnits(TemperatureUnits::Celsius);

	BOOST_CHECK_EQUAL(0, events);

	connection.disconnect();
}

BOOST_AUTO_TEST_SUITE_END()
