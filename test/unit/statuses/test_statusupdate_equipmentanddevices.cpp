#include <boost/signals2.hpp>
#include <boost/test/unit_test.hpp>

#include <functional>
#include <memory>
#include <string_view>

#include "devices/device_status.h"
#include "equipment/equipment_status.h"
#include "interfaces/istatuspublisher.h"

using namespace AqualinkAutomate;

BOOST_AUTO_TEST_SUITE(TestSuite_EquipmentAndDevices_StatusUpdates)

BOOST_AUTO_TEST_CASE(Test_EquipmentAndDevices_StatusUpdates_IStatus)
{
    Devices::DeviceStatus_Initializing device_status_initializing;
    Devices::DeviceStatus_Normal device_status_normal;

    BOOST_CHECK_EQUAL(device_status_initializing.StatusType(), "Initializing");
    BOOST_CHECK_EQUAL(device_status_normal.StatusType(), "Normal");

    // Type-only "same status type" relation is now a named predicate rather than
    // the previous value-blind operator==/operator!=.
    BOOST_CHECK((!Interfaces::IsSameStatusType<Devices::DeviceStatus_Initializing, Devices::DeviceStatus_Normal>()));
    BOOST_CHECK((Interfaces::IsSameStatusType<Devices::DeviceStatus_Initializing, Devices::DeviceStatus_Initializing>()));
}

BOOST_AUTO_TEST_CASE(Test_EquipmentAndDevices_StatusUpdates_IStatusPublisher)
{
    Devices::DeviceStatus_Initializing device_status_initializing;
    Devices::DeviceStatus_Normal device_status_normal;

    Interfaces::IStatusPublisher status_publisher(device_status_initializing);

    BOOST_CHECK_EQUAL(status_publisher.Status().lock()->StatusType(), "Initializing");

    bool signal_received = false;
    std::string received_status_name;

    auto connection = status_publisher.StatusSignal.connect
    (
        [&signal_received, &received_status_name](const Interfaces::IStatusPublisher::StatusType& status)
        {
            if (auto status_ptr = status.lock(); nullptr != status_ptr)
            {
                received_status_name = status_ptr->StatusType();
                signal_received = true;
            }
            
        }
    );

    status_publisher.Status(device_status_normal);

    BOOST_CHECK(signal_received);
    BOOST_CHECK_EQUAL(received_status_name, "Normal");

    BOOST_CHECK_EQUAL(status_publisher.Status().lock()->StatusType(), "Normal");
}

BOOST_AUTO_TEST_CASE(Test_EquipmentAndDevices_StatusUpdates_EmitsOncePerDistinctStatusType)
{
    // Regression: devices re-publish their status on every poll. IStatusPublisher must only fan
    // out the StatusSignal when the status type actually changes, not on every repeated publish.
    Interfaces::IStatusPublisher status_publisher(Devices::DeviceStatus_Initializing{});

    int emit_count = 0;
    auto connection = status_publisher.StatusSignal.connect(
        [&emit_count](const Interfaces::IStatusPublisher::StatusType&)
        {
            ++emit_count;
        });

    // Re-publishing the same status type (the every-poll case) must be silent.
    status_publisher.Status(Devices::DeviceStatus_Initializing{});
    status_publisher.Status(Devices::DeviceStatus_Initializing{});
    BOOST_CHECK_EQUAL(emit_count, 0);

    // A genuine change fires exactly once; repeating it is silent again.
    status_publisher.Status(Devices::DeviceStatus_Normal{});
    status_publisher.Status(Devices::DeviceStatus_Normal{});
    BOOST_CHECK_EQUAL(emit_count, 1);

    // A third distinct status type fires again, and the latest status is retained.
    status_publisher.Status(Devices::DeviceStatus_Unknown{});
    BOOST_CHECK_EQUAL(emit_count, 2);
    BOOST_CHECK_EQUAL(status_publisher.Status().lock()->StatusType(), "Unknown");
}

BOOST_AUTO_TEST_CASE(Test_EquipmentAndDevices_StatusUpdates_DeviceStatus)
{
    Devices::DeviceStatus_Initializing device_status;
    BOOST_CHECK_EQUAL(device_status.StatusType(), "Initializing");
}

BOOST_AUTO_TEST_CASE(Test_EquipmentAndDevices_StatusUpdates_EquipmentStatus)
{
    Equipment::EquipmentStatus_Unknown equipment_status;
    BOOST_CHECK_EQUAL(equipment_status.StatusType(), "Unknown");
}

BOOST_AUTO_TEST_CASE(Test_EquipmentAndDevices_StatusUpdates_IsSameStatusType)
{
    // Type-only relation: same type -> true, different type -> false.
    BOOST_CHECK((Interfaces::IsSameStatusType<Devices::DeviceStatus_Normal, Devices::DeviceStatus_Normal>()));
    BOOST_CHECK((!Interfaces::IsSameStatusType<Devices::DeviceStatus_Normal, Devices::DeviceStatus_Initializing>()));

    // Crosses categories: device vs equipment statuses are never the same type.
    BOOST_CHECK((!Interfaces::IsSameStatusType<Devices::DeviceStatus_Unknown, Equipment::EquipmentStatus_Unknown>()));

    // It is a compile-time constant expression.
    static_assert(Interfaces::IsSameStatusType<Devices::DeviceStatus_Normal, Devices::DeviceStatus_Normal>());
    static_assert(!Interfaces::IsSameStatusType<Devices::DeviceStatus_Normal, Devices::DeviceStatus_Unknown>());
}

BOOST_AUTO_TEST_CASE(Test_EquipmentAndDevices_StatusUpdates_PublishedStatusCategoryCompare)
{
    // operator== / operator!= against the published (weak_ptr) status do REAL
    // category equality: they must reflect the type currently held by the publisher.
    Devices::DeviceStatus_Initializing device_status_initializing;
    Interfaces::IStatusPublisher status_publisher(device_status_initializing);

    BOOST_CHECK(Devices::DeviceStatus_Initializing{} == status_publisher.Status());
    BOOST_CHECK(!(Devices::DeviceStatus_Normal{} == status_publisher.Status()));
    BOOST_CHECK(Devices::DeviceStatus_Normal{} != status_publisher.Status());

    status_publisher.Status(Devices::DeviceStatus_Normal{});

    BOOST_CHECK(Devices::DeviceStatus_Normal{} == status_publisher.Status());
    BOOST_CHECK(Devices::DeviceStatus_Initializing{} != status_publisher.Status());
}

BOOST_AUTO_TEST_CASE(Test_EquipmentAndDevices_StatusUpdates_SourceTypeAndName)
{
    // SourceType() still reflects the category; SourceName() no longer returns the
    // old frozen "my_device"/"my_source" placeholders.
    Devices::DeviceStatus_Normal device_status;
    BOOST_CHECK_EQUAL(device_status.SourceType(), "device");
    BOOST_CHECK_NE(device_status.SourceName(), "my_device");

    Equipment::EquipmentStatus_Unknown equipment_status;
    BOOST_CHECK_EQUAL(equipment_status.SourceType(), "equipment");
    BOOST_CHECK_NE(equipment_status.SourceName(), "my_source");
}

BOOST_AUTO_TEST_SUITE_END()
