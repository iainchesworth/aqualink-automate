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
#include "kernel/auxillary_devices/stable_id.h"
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

BOOST_AUTO_TEST_CASE(Snapshot_PersistsHardwareId_AndApplyRestoresIt)
{
	boost::asio::io_context io;
	Options::Equipment::EquipmentSettings settings;
	EquipmentCache::EquipmentCacheService source(io, *this, settings);

	auto hub = Find<Kernel::DataHub>();
	auto dev = MakeDevice("Swim Jet", Traits::AuxillaryTypes::Auxillary);
	dev->AuxillaryTraits.Set(Traits::HardwareLabelTrait{}, std::string{ "Aux5" });
	hub->Devices.Add(dev);

	const auto snapshot = source.Snapshot();
	BOOST_REQUIRE_EQUAL(snapshot["devices"].size(), 1u);
	BOOST_CHECK_EQUAL(snapshot["devices"][0]["hardware_id"], "Aux5");

	Test::HubLocatorInjector fresh;
	EquipmentCache::EquipmentCacheService dest(io, fresh, settings);
	dest.ApplySnapshot(snapshot);

	auto restored = fresh.Find<Kernel::DataHub>()->Devices.FindByLabel("Swim Jet");
	BOOST_REQUIRE_EQUAL(restored.size(), 1u);
	BOOST_REQUIRE(restored.front()->AuxillaryTraits.Has(Traits::HardwareLabelTrait{}));
	BOOST_CHECK_EQUAL(std::string{ *(restored.front()->AuxillaryTraits[Traits::HardwareLabelTrait{}]) }, "Aux5");
}

// Distinct auxes round-trip and each is reachable by the EXACT stable id live discovery
// reconciles on - this is what lets a cache-restored placeholder be found (and updated in
// place) rather than duplicated. (Same-LABEL devices collapse on restore via the device
// graph's type+label dedup; the no-duplicate guarantee for that case is proven end-to-end in
// test_navigation_aux_scrolling_discovery's Replay_LabelAux_AfterCacheRestore test, where
// live discovery reconciles by stable id.)
BOOST_AUTO_TEST_CASE(ApplySnapshot_DistinctDevices_RestoreWithStableIdAndHardwareId)
{
	boost::asio::io_context io;
	Options::Equipment::EquipmentSettings settings;
	EquipmentCache::EquipmentCacheService source(io, *this, settings);

	struct Entry { const char* key; const char* label; const char* hardware; };
	const Entry entries[] = {
		{ "jandy:aux:1", "Swim Jet",  "Aux1" },
		{ "jandy:aux:2", "Spa Jet",   "Aux2" },
		{ "jandy:aux:3", "Waterfall", "Aux3" },
	};

	auto hub = Find<Kernel::DataHub>();
	for (const auto& e : entries)
	{
		auto dev = std::make_shared<Kernel::AuxillaryDevice>(Kernel::DeriveStableUuid(e.key));
		dev->AuxillaryTraits.Set(Traits::AuxillaryTypeTrait{}, Traits::AuxillaryTypes::Auxillary);
		dev->AuxillaryTraits.Set(Traits::LabelTrait{}, std::string{ e.label });
		dev->AuxillaryTraits.Set(Traits::HardwareLabelTrait{}, std::string{ e.hardware });
		hub->Devices.Add(dev);
	}

	const auto snapshot = source.Snapshot();
	BOOST_REQUIRE_EQUAL(snapshot["devices"].size(), 3u);

	Test::HubLocatorInjector fresh;
	EquipmentCache::EquipmentCacheService dest(io, fresh, settings);
	dest.ApplySnapshot(snapshot);

	auto fresh_hub = fresh.Find<Kernel::DataHub>();
	BOOST_CHECK_EQUAL(fresh_hub->Devices.FindByLabel("Swim Jet").size(), 1u);
	BOOST_CHECK_EQUAL(fresh_hub->Devices.FindByLabel("Spa Jet").size(), 1u);

	// Each restored device is reachable by the exact stable id live discovery computes.
	auto restored = fresh_hub->Devices.FindById(Kernel::DeriveStableUuid("jandy:aux:2"));
	BOOST_REQUIRE(nullptr != restored);
	BOOST_REQUIRE(restored->AuxillaryTraits.Has(Traits::HardwareLabelTrait{}));
	BOOST_CHECK_EQUAL(std::string{ *(restored->AuxillaryTraits[Traits::HardwareLabelTrait{}]) }, "Aux2");
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
