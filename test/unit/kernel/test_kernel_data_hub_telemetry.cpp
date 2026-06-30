#include <functional>
#include <memory>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include "kernel/data_hub.h"
#include "kernel/hub_events/data_hub_config_event.h"
#include "kernel/hub_events/data_hub_config_event_chemistry.h"
#include "kernel/hub_events/data_hub_config_event_temperature.h"
#include "kernel/hub_events/hub_eventtypes.h"
#include "kernel/orp.h"
#include "kernel/ph.h"
#include "kernel/temperature.h"
#include "types/units_dimensionless.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Kernel;

// Regression coverage for the redundant-publish bug: the controller re-reports temperatures and
// chemistry on every status poll, but the DataHub setters must only fan out a ConfigUpdateSignal
// when the underlying value actually changes (mirroring SetCirculationMode). A repeated identical
// reading must be silent so downstream WebSocket/MQTT consumers are not spammed with no-op updates.

namespace
{
	int CountEventsOfType(DataHub& hub, Hub_EventTypes type, const std::function<void()>& actions)
	{
		int count = 0;
		auto connection = hub.ConfigUpdateSignal.connect(
			[&count, type](const std::shared_ptr<DataHub_ConfigEvent>& ev)
			{
				if (ev && ev->Type() == type)
				{
					++count;
				}
			});

		actions();
		connection.disconnect();
		return count;
	}
}

BOOST_AUTO_TEST_SUITE(DataHub_Telemetry_TestSuite)

BOOST_AUTO_TEST_CASE(PoolTemp_EmitsOncePerDistinctValue)
{
	DataHub hub;

	const int events = CountEventsOfType(hub, Hub_EventTypes::Temperature,
		[&hub]()
		{
			// First reading (from the nullopt default) is a change; the two identical re-reports
			// that follow must be silent; the new value fires exactly once more.
			hub.PoolTemp(Temperature::ConvertToTemperatureInCelsius(28.0));
			hub.PoolTemp(Temperature::ConvertToTemperatureInCelsius(28.0));
			hub.PoolTemp(Temperature::ConvertToTemperatureInCelsius(28.0));
			hub.PoolTemp(Temperature::ConvertToTemperatureInCelsius(29.0));
		});

	BOOST_CHECK_EQUAL(events, 2);

	// The latest value is retained even across the suppressed (no-emit) re-reports.
	BOOST_REQUIRE(hub.PoolTemp().has_value());
	BOOST_CHECK_CLOSE(hub.PoolTemp()->InCelsius().value(), 29.0, 0.001);
}

BOOST_AUTO_TEST_CASE(AirTemp_NoReEmitWhenUnchanged)
{
	DataHub hub;

	const int events = CountEventsOfType(hub, Hub_EventTypes::Temperature,
		[&hub]()
		{
			hub.AirTemp(Temperature::ConvertToTemperatureInCelsius(21.5));
			hub.AirTemp(Temperature::ConvertToTemperatureInCelsius(21.5));
		});

	BOOST_CHECK_EQUAL(events, 1);
}

BOOST_AUTO_TEST_CASE(PoolTempSetpoint_NoReEmitWhenUnchanged)
{
	DataHub hub;

	const int events = CountEventsOfType(hub, Hub_EventTypes::Temperature,
		[&hub]()
		{
			hub.PoolTempSetpoint(Temperature::ConvertToTemperatureInCelsius(30.0));
			hub.PoolTempSetpoint(Temperature::ConvertToTemperatureInCelsius(30.0));
			hub.PoolTempSetpoint(Temperature::ConvertToTemperatureInCelsius(31.0));
		});

	BOOST_CHECK_EQUAL(events, 2);
}

BOOST_AUTO_TEST_CASE(ORP_EmitsOncePerDistinctValue)
{
	DataHub hub;

	const int events = CountEventsOfType(hub, Hub_EventTypes::Chemistry,
		[&hub]()
		{
			hub.ORP(ORP(750.0));
			hub.ORP(ORP(750.0));
			hub.ORP(ORP(760.0));
		});

	BOOST_CHECK_EQUAL(events, 2);
}

BOOST_AUTO_TEST_CASE(pH_EmitsOncePerDistinctValue)
{
	DataHub hub;

	const int events = CountEventsOfType(hub, Hub_EventTypes::Chemistry,
		[&hub]()
		{
			hub.pH(pH(7.4f));
			hub.pH(pH(7.4f));
			hub.pH(pH(7.6f));
		});

	BOOST_CHECK_EQUAL(events, 2);
}

BOOST_AUTO_TEST_CASE(SaltLevel_NoReEmitWhenUnchanged)
{
	DataHub hub;

	const int events = CountEventsOfType(hub, Hub_EventTypes::Chemistry,
		[&hub]()
		{
			hub.SaltLevel(3200.0 * Units::ppm);
			hub.SaltLevel(3200.0 * Units::ppm);
			hub.SaltLevel(3300.0 * Units::ppm);
		});

	BOOST_CHECK_EQUAL(events, 2);
}

BOOST_AUTO_TEST_CASE(EmitButtonStateChange_EmitsOncePerDistinctState)
{
	DataHub hub;
	boost::uuids::string_generator gen;
	const auto button = gen("01234567-89ab-cdef-0123-456789abcdef");

	const int events = CountEventsOfType(hub, Hub_EventTypes::ButtonStateChange,
		[&hub, &button]()
		{
			// Re-scraping the same button state every poll must be silent after the first emit.
			hub.EmitButtonStateChange(button, "Running", "Filter Pump");
			hub.EmitButtonStateChange(button, "Running", "Filter Pump");
			hub.EmitButtonStateChange(button, "Running", "Filter Pump");
			// A genuine status change fires once more...
			hub.EmitButtonStateChange(button, "Off", "Filter Pump");
			// ...and a label-only change is also a real change.
			hub.EmitButtonStateChange(button, "Off", "Pool Pump");
		});

	BOOST_CHECK_EQUAL(events, 3);
}

BOOST_AUTO_TEST_CASE(EmitButtonStateChange_TracksButtonsIndependently)
{
	DataHub hub;
	boost::uuids::string_generator gen;
	const auto button_a = gen("01234567-89ab-cdef-0123-456789abcdef");
	const auto button_b = gen("fedcba98-7654-3210-fedc-ba9876543210");

	const int events = CountEventsOfType(hub, Hub_EventTypes::ButtonStateChange,
		[&hub, &button_a, &button_b]()
		{
			// Two different buttons reporting the same status string are distinct states; each
			// fires once. Re-reporting either unchanged is then silent.
			hub.EmitButtonStateChange(button_a, "Running", "Pump A");
			hub.EmitButtonStateChange(button_b, "Running", "Pump B");
			hub.EmitButtonStateChange(button_a, "Running", "Pump A");
			hub.EmitButtonStateChange(button_b, "Running", "Pump B");
		});

	BOOST_CHECK_EQUAL(events, 2);
}

BOOST_AUTO_TEST_SUITE_END()
