#include <string>
#include <vector>

#include <boost/any.hpp>
#include <boost/program_options.hpp>
#include <boost/test/unit_test.hpp>

#include "jandy/devices/jandy_device_id.h"
#include "jandy/formatters/jandy_device_formatters.h"
#include "options/validators/jandy_device_id_validator.h"

#include "support/unit_test_ostream_support.h"

BOOST_AUTO_TEST_SUITE(LoggingValidator_JandyDeviceId);

BOOST_AUTO_TEST_CASE(ValidDeviceID)
{
    using namespace AqualinkAutomate::Devices;

    const std::vector<std::string> device_ids = { "0x00", "0x0A", "0xFF", "0x3F", "0xB2" };
    const std::vector<JandyDeviceId> expected_device_ids = { JandyDeviceId(0x00), JandyDeviceId(0x0A), JandyDeviceId(0xFF), JandyDeviceId(0x3F), JandyDeviceId(0xB2) };

    for (int i = 0; i < device_ids.size(); ++i) 
    {
        boost::any val;
        JandyDeviceId target(0xCC); // Not a specific test value.
        std::vector<std::string> values { device_ids[i] };

        validate(val, values, &target, 0);

        BOOST_CHECK_EQUAL(boost::any_cast<JandyDeviceId>(val), expected_device_ids[i]);
    }
}

BOOST_AUTO_TEST_CASE(InvalidDeviceID)
{
    using namespace AqualinkAutomate::Devices;

    const std::vector<std::string> invalid_device_ids = { "00", "0x1G", "-10", "0x100", "0xFFF" };
    for (const auto& device_id : invalid_device_ids) 
    {
        boost::any val;
        JandyDeviceId target(0xCC); // Not a specific test value.
        std::vector<std::string> values { device_id };

        BOOST_CHECK_THROW(validate(val, values, &target, 0), boost::program_options::validation_error);
    }
}

BOOST_AUTO_TEST_SUITE_END();
