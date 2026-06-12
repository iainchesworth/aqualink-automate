#include <filesystem>
#include <memory>
#include <string>
#include <system_error>

#include <boost/test/unit_test.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <nlohmann/json.hpp>

#include "equipment_cache/equipment_cache_service.h"
#include "kernel/auxillary_devices/auxillary_device.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "kernel/data_hub.h"
#include "options/options_equipment_options.h"

#include "support/unit_test_hublocatorinjector.h"

using namespace AqualinkAutomate;
namespace Traits = Kernel::AuxillaryTraitsTypes;

namespace
{
	std::shared_ptr<Kernel::AuxillaryDevice> MakeDevice(const std::string& label, Traits::AuxillaryTypes type)
	{
		auto dev = std::make_shared<Kernel::AuxillaryDevice>();
		dev->AuxillaryTraits.Set(Traits::AuxillaryTypeTrait{}, type);
		dev->AuxillaryTraits.Set(Traits::LabelTrait{}, label);
		return dev;
	}
}

BOOST_FIXTURE_TEST_SUITE(TestSuite_EquipmentCache, Test::HubLocatorInjector)

BOOST_AUTO_TEST_CASE(SnapshotApply_RoundTripsConfigAndDevicesWithStableUuid)
{
	boost::asio::io_context io;
	Options::Equipment::EquipmentSettings settings;   // no file => in-memory
	EquipmentCache::EquipmentCacheService source(io, *this, settings);

	auto hub = Find<Kernel::DataHub>();
	hub->ApplyPoolConfiguration(Kernel::PoolConfigurations::SingleBody, Kernel::ConfigurationSource::Auto);

	auto dev = MakeDevice("Aux1", Traits::AuxillaryTypes::Light);
	dev->AuxillaryTraits.Set(Traits::BodyOfWaterTrait{}, Kernel::BodyOfWaterIds::Pool);
	const auto original_id = dev->Id();
	hub->Devices.Add(dev);

	const auto snapshot = source.Snapshot();
	BOOST_CHECK_EQUAL(snapshot["pool_configuration"], "SingleBody");
	BOOST_REQUIRE_EQUAL(snapshot["devices"].size(), 1u);
	BOOST_CHECK_EQUAL(snapshot["devices"][0]["label"], "Aux1");
	BOOST_CHECK_EQUAL(snapshot["devices"][0]["type"], "Light");

	// Apply into a fresh hub: config + device restored, UUID preserved.
	Test::HubLocatorInjector fresh;
	EquipmentCache::EquipmentCacheService dest(io, fresh, settings);
	dest.ApplySnapshot(snapshot);

	auto fresh_hub = fresh.Find<Kernel::DataHub>();
	BOOST_CHECK(fresh_hub->PoolConfiguration == Kernel::PoolConfigurations::SingleBody);

	auto restored = fresh_hub->Devices.FindByLabel("Aux1");
	BOOST_REQUIRE_EQUAL(restored.size(), 1u);
	BOOST_CHECK(restored.front()->Id() == original_id);   // stable id across "restart"
	BOOST_CHECK(restored.front()->AuxillaryTraits.Has(Traits::AuxillaryTypeTrait{}));
}

BOOST_AUTO_TEST_CASE(ApplySnapshot_MergesByLabel_NoDuplicate)
{
	boost::asio::io_context io;
	Options::Equipment::EquipmentSettings settings;
	EquipmentCache::EquipmentCacheService service(io, *this, settings);

	auto hub = Find<Kernel::DataHub>();
	hub->Devices.Add(MakeDevice("Aux1", Traits::AuxillaryTypes::Auxillary));

	// A cached snapshot for the SAME label (different uuid) must not duplicate it.
	nlohmann::json snapshot;
	snapshot["devices"] = nlohmann::json::array({
		{ { "id", boost::uuids::to_string(boost::uuids::random_generator()()) }, { "label", "Aux1" }, { "type", "Auxillary" } }
	});
	service.ApplySnapshot(snapshot);

	BOOST_CHECK_EQUAL(hub->Devices.FindByLabel("Aux1").size(), 1u);
}

BOOST_AUTO_TEST_CASE(FileRoundTrip_SaveThenLoad)
{
	boost::asio::io_context io;
	const auto path = (std::filesystem::temp_directory_path() / "aqualink_eqcache_test.json").string();
	std::error_code ec;
	std::filesystem::remove(path, ec);

	Options::Equipment::EquipmentSettings settings;
	settings.equipment_cache_file = path;

	{
		EquipmentCache::EquipmentCacheService writer(io, *this, settings);
		Find<Kernel::DataHub>()->Devices.Add(MakeDevice("Filter Pump", Traits::AuxillaryTypes::Pump));
		writer.SaveNow();
	}

	Test::HubLocatorInjector fresh;
	EquipmentCache::EquipmentCacheService reader(io, fresh, settings);
	reader.Load();
	BOOST_CHECK_EQUAL(fresh.Find<Kernel::DataHub>()->Devices.FindByLabel("Filter Pump").size(), 1u);

	std::filesystem::remove(path, ec);
	std::filesystem::remove(path + ".tmp", ec);
}

BOOST_AUTO_TEST_SUITE_END()
