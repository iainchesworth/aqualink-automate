#include <string>
#include <vector>

#include <boost/any.hpp>
#include <boost/program_options.hpp>
#include <boost/test/unit_test.hpp>

#include "jandy/devices/jandy_emulated_device_types.h"
#include "options/validators/jandy_emulated_device_type_validator.h"

#include "support/unit_test_ostream_support.h"

BOOST_AUTO_TEST_SUITE(LoggingValidator_EmulatedJandyDeviceType);

BOOST_AUTO_TEST_CASE(ValidEmulatedDevice)
{
    using namespace AqualinkAutomate::Devices;

    // Test all emulated device types in various cases
    const std::vector<std::string> device_types = { "rs_keypad", "ONETOUCH", "Iaq", "PdA", "UnKnOwn" };
    const std::vector<JandyEmulatedDeviceTypes> expected_device_types = { JandyEmulatedDeviceTypes::RS_Keypad, JandyEmulatedDeviceTypes::OneTouch, JandyEmulatedDeviceTypes::IAQ, JandyEmulatedDeviceTypes::PDA, JandyEmulatedDeviceTypes::Unknown };

    for (int i = 0; i < device_types.size(); ++i) 
    {
        boost::any val;
        JandyEmulatedDeviceTypes target;
		std::vector<std::string> values { device_types[i] };

        validate(val, values, &target, 0);

        BOOST_CHECK_EQUAL(boost::any_cast<JandyEmulatedDeviceTypes>(val), expected_device_types[i]);
    }
}

BOOST_AUTO_TEST_CASE(InvalidEmulatedDevice)
{
    using namespace AqualinkAutomate::Devices;

    boost::any val;
    std::vector<std::string> values;
    JandyEmulatedDeviceTypes target;

    // Test invalid emulated device
    values.push_back("invalid_device");
    BOOST_CHECK_THROW(validate(val, values, &target, 0), boost::program_options::validation_error);
}

BOOST_AUTO_TEST_SUITE_END();
