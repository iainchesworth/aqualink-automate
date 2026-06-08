#include <memory>
#include <string>

#include <boost/test/unit_test.hpp>

#include "interfaces/idevice.h"
#include "interfaces/ideviceidentifier.h"
#include "interfaces/iequipment.h"
#include "kernel/equipment_hub.h"

using namespace AqualinkAutomate::Interfaces;
using namespace AqualinkAutomate::Kernel;

namespace
{
	// -------------------------------------------------------------------------
	// Minimal IEquipment subclasses.  Two distinct concrete types are required
	// to reproduce the historical "only one IEquipment can ever register" bug
	// (Jandy + Pentair both being IEquipment subclasses in production).
	// -------------------------------------------------------------------------

	class TestEquipmentA : public IEquipment {};
	class TestEquipmentB : public IEquipment {};

	// -------------------------------------------------------------------------
	// A trivial device identifier wrapping an integer.  Two distinct concrete
	// identifier types exercise the bucket-by-identifier-type device store.
	// -------------------------------------------------------------------------

	class TestDeviceIdA : public IDeviceIdentifier
	{
	public:
		explicit TestDeviceIdA(int id) : m_Id(id) {}

	protected:
		bool Equals(const IDeviceIdentifier& other) const override
		{
			const auto* typed = dynamic_cast<const TestDeviceIdA*>(&other);
			return (nullptr != typed) && (typed->m_Id == m_Id);
		}

	private:
		int m_Id;
	};

	class TestDeviceIdB : public IDeviceIdentifier
	{
	public:
		explicit TestDeviceIdB(int id) : m_Id(id) {}

	protected:
		bool Equals(const IDeviceIdentifier& other) const override
		{
			const auto* typed = dynamic_cast<const TestDeviceIdB*>(&other);
			return (nullptr != typed) && (typed->m_Id == m_Id);
		}

	private:
		int m_Id;
	};

	class TestDevice : public IDevice
	{
	public:
		explicit TestDevice(std::shared_ptr<IDeviceIdentifier> device_id) :
			IDevice(std::move(device_id))
		{
		}
	};

	std::unique_ptr<TestDevice> MakeDeviceA(int id)
	{
		return std::make_unique<TestDevice>(std::make_shared<TestDeviceIdA>(id));
	}

	std::unique_ptr<TestDevice> MakeDeviceB(int id)
	{
		return std::make_unique<TestDevice>(std::make_shared<TestDeviceIdB>(id));
	}
}

BOOST_AUTO_TEST_SUITE(EquipmentHub_TestSuite)

// =============================================================================
// Equipment keyed by runtime pointee type (the Pentair-drop regression)
// =============================================================================

BOOST_AUTO_TEST_CASE(AddEquipment_TwoDistinctSubclasses_BothRegister)
{
	EquipmentHub hub;

	BOOST_CHECK(hub.AddEquipment(std::make_unique<TestEquipmentA>()));
	// Regression: previously this was silently rejected as a duplicate
	// because both subclasses keyed onto typeid(IEquipment).
	BOOST_CHECK(hub.AddEquipment(std::make_unique<TestEquipmentB>()));
}

BOOST_AUTO_TEST_CASE(AddEquipment_SameSubclassTwice_SecondRejected)
{
	EquipmentHub hub;

	BOOST_CHECK(hub.AddEquipment(std::make_unique<TestEquipmentA>()));
	BOOST_CHECK(!hub.AddEquipment(std::make_unique<TestEquipmentA>()));
}

BOOST_AUTO_TEST_CASE(AddEquipment_Null_ReturnsFalse)
{
	EquipmentHub hub;

	std::unique_ptr<IEquipment> null_equipment;
	BOOST_CHECK(!hub.AddEquipment(std::move(null_equipment)));
}

// =============================================================================
// Device registration / existence
// =============================================================================

BOOST_AUTO_TEST_CASE(AddDevice_ThenExists_ByIdentifier)
{
	EquipmentHub hub;

	BOOST_CHECK(hub.AddDevice(MakeDeviceA(1)));

	TestDeviceIdA query(1);
	BOOST_CHECK(hub.DeviceExists(query));

	TestDeviceIdA missing(2);
	BOOST_CHECK(!hub.DeviceExists(missing));
}

BOOST_AUTO_TEST_CASE(AddDevice_DuplicateId_SecondRejected)
{
	EquipmentHub hub;

	BOOST_CHECK(hub.AddDevice(MakeDeviceA(1)));
	BOOST_CHECK(!hub.AddDevice(MakeDeviceA(1)));
}

BOOST_AUTO_TEST_CASE(AddDevice_Null_ReturnsFalse)
{
	EquipmentHub hub;

	std::unique_ptr<IDevice> null_device;
	BOOST_CHECK(!hub.AddDevice(std::move(null_device)));
}

BOOST_AUTO_TEST_CASE(DeviceExists_DistinctIdentifierTypesSameValue_DoNotCollide)
{
	EquipmentHub hub;

	// Same integer value but different identifier *types* must be treated as
	// distinct devices (the bucket key is the identifier's runtime type).
	BOOST_CHECK(hub.AddDevice(MakeDeviceA(1)));
	BOOST_CHECK(hub.AddDevice(MakeDeviceB(1)));

	TestDeviceIdA query_a(1);
	TestDeviceIdB query_b(1);
	BOOST_CHECK(hub.DeviceExists(query_a));
	BOOST_CHECK(hub.DeviceExists(query_b));
}

// =============================================================================
// FindDevice / ForEachDevice
// =============================================================================

BOOST_AUTO_TEST_CASE(FindDevice_MatchingPredicate_ReturnsDevice)
{
	EquipmentHub hub;
	BOOST_REQUIRE(hub.AddDevice(MakeDeviceA(7)));

	TestDeviceIdA needle(7);
	auto* found = hub.FindDevice([&needle](const IDevice& dev)
		{
			return dev.DeviceId() == needle;
		});

	BOOST_CHECK(nullptr != found);
}

BOOST_AUTO_TEST_CASE(FindDevice_NoMatch_ReturnsNullptr)
{
	EquipmentHub hub;
	BOOST_REQUIRE(hub.AddDevice(MakeDeviceA(7)));

	auto* found = hub.FindDevice([](const IDevice&) { return false; });
	BOOST_CHECK(nullptr == found);
}

BOOST_AUTO_TEST_CASE(ForEachDevice_VisitsAllRegisteredDevices)
{
	EquipmentHub hub;
	BOOST_REQUIRE(hub.AddDevice(MakeDeviceA(1)));
	BOOST_REQUIRE(hub.AddDevice(MakeDeviceA(2)));
	BOOST_REQUIRE(hub.AddDevice(MakeDeviceB(1)));

	int visited = 0;
	hub.ForEachDevice([&visited](const IDevice&) { ++visited; });

	BOOST_CHECK_EQUAL(visited, 3);
}

BOOST_AUTO_TEST_SUITE_END()
