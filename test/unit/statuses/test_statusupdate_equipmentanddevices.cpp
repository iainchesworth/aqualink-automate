#include <boost/signals2.hpp>
#include <boost/test/unit_test.hpp>

#include <functional>
#include <memory>
#include <string_view>

#include "devices/device_status.h"
#include "equipment/equipment_status.h"
#include "interfaces/istatuspublisher.h"

using namespace AqualinkAutomate;

BOOST_AUTO_TEST_SUITE(EquipmentAndDevices_StatusUpdates_TestSuite)

BOOST_AUTO_TEST_CASE(TestIStatus)
{
    Devices::DeviceStatus_Initializing device_status_initializing;
    Devices::DeviceStatus_Normal device_status_normal;

    BOOST_CHECK_EQUAL(device_status_initializing.Name(), "Initializing");
    BOOST_CHECK_EQUAL(device_status_normal.Name(), "Normal");

    BOOST_CHECK(device_status_initializing != device_status_normal);
}

BOOST_AUTO_TEST_CASE(TestIStatusPublisher)
{
    Devices::DeviceStatus_Initializing device_status_initializing;
    Devices::DeviceStatus_Normal device_status_normal;

    Interfaces::IStatusPublisher status_publisher(std::move(device_status_initializing));

    BOOST_CHECK_EQUAL(status_publisher.Status().lock()->Name(), "Initializing");

    bool signal_received = false;
    std::string received_status_name;

    auto connection = status_publisher.StatusSignal.connect
    (
        [&signal_received, &received_status_name](Interfaces::IStatusPublisher::StatusType status)
        {
            if (auto status_ptr = status.lock(); nullptr != status_ptr)
            {
                received_status_name = status_ptr->Name();
                signal_received = true;
            }
            
        }
    );

    status_publisher.Status(std::move(device_status_normal));

    BOOST_CHECK(signal_received);
    BOOST_CHECK_EQUAL(received_status_name, "Normal");

    BOOST_CHECK_EQUAL(status_publisher.Status().lock()->Name(), "Normal");
}

BOOST_AUTO_TEST_CASE(TestDeviceStatus)
{
    Devices::DeviceStatus_Initializing device_status;
    BOOST_CHECK_EQUAL(device_status.Name(), "Initializing");
}

BOOST_AUTO_TEST_CASE(TestEquipmentStatus)
{
    Equipment::EquipmentStatus_Unknown equipment_status;
    BOOST_CHECK_EQUAL(equipment_status.Name(), "Unknown");
}

BOOST_AUTO_TEST_SUITE_END()
