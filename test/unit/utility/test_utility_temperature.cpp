#include <boost/test/unit_test.hpp>

#include "formatters/temperature_formatter.h"
#include "jandy/formatters/temperature_formatter.h"
#include "jandy/utility/string_conversion/temperature_string_converter.h"

#include "support/unit_test_ostream_support.h"

using namespace AqualinkAutomate::Utility;

BOOST_AUTO_TEST_SUITE(TestSuite_Temperature);

BOOST_AUTO_TEST_CASE(Test_Temperature_ValidTemperatureStrings)
{
    {
        TemperatureStringConverter temp("pool 22`C");
        BOOST_REQUIRE(temp().has_value());
        BOOST_CHECK(22.0f == temp().value().InCelsius().value());
        BOOST_REQUIRE(temp.TemperatureArea().has_value());
        BOOST_CHECK("pool" == *(temp.TemperatureArea()));
    }

    {
        TemperatureStringConverter temp("air 72`F");
        BOOST_REQUIRE(temp().has_value());
        BOOST_CHECK(72.0f == temp().value().InFahrenheit().value());
        BOOST_REQUIRE(temp.TemperatureArea().has_value());
        BOOST_CHECK("air" == *(temp.TemperatureArea()));
    }

    {
        TemperatureStringConverter temp("Pool 22`C");
        BOOST_REQUIRE(temp().has_value());
        BOOST_CHECK(22.0f == temp().value().InCelsius().value());
        BOOST_REQUIRE(temp.TemperatureArea().has_value());
        BOOST_CHECK("Pool" == *(temp.TemperatureArea()));
    }

    {
        TemperatureStringConverter temp("Air 18`C");
        BOOST_REQUIRE(temp().has_value());
        BOOST_CHECK(18.0f == temp().value().InCelsius().value());
        BOOST_REQUIRE(temp.TemperatureArea().has_value());
        BOOST_CHECK("Air" == *(temp.TemperatureArea()));
    }

    {
        TemperatureStringConverter temp("Air -18`C");
        BOOST_REQUIRE(temp().has_value());
        BOOST_CHECK(-18.0f == temp().value().InCelsius().value());
        BOOST_REQUIRE(temp.TemperatureArea().has_value());
        BOOST_CHECK("Air" == *(temp.TemperatureArea()));
    }

    {
        TemperatureStringConverter temp("Pool 0`C");
        BOOST_REQUIRE(temp().has_value());
        BOOST_CHECK(0.0f == temp().value().InCelsius().value());
        BOOST_REQUIRE(temp.TemperatureArea().has_value());
        BOOST_CHECK("Pool" == *(temp.TemperatureArea()));
    }

    {
        TemperatureStringConverter temp("pool        22`C");
        BOOST_REQUIRE(temp().has_value());
        BOOST_CHECK(22.0f == temp().value().InCelsius().value());
        BOOST_REQUIRE(temp.TemperatureArea().has_value());
        BOOST_CHECK("pool" == *(temp.TemperatureArea()));
    }

    {
        TemperatureStringConverter temp("pool        72`F");
        BOOST_REQUIRE(temp().has_value());
        BOOST_CHECK(72.0f == temp().value().InFahrenheit().value());
        BOOST_REQUIRE(temp.TemperatureArea().has_value());
        BOOST_CHECK("pool" == *(temp.TemperatureArea()));
    }

    {
        TemperatureStringConverter temp("someth 18`C");
        BOOST_REQUIRE(temp().has_value());
        BOOST_CHECK(18.0f == temp().value().InCelsius().value());
        BOOST_REQUIRE(temp.TemperatureArea().has_value());
        BOOST_CHECK("someth" == *(temp.TemperatureArea()));
    }

    {
        TemperatureStringConverter temp("Pool        22`C");
        BOOST_REQUIRE(temp().has_value());
        BOOST_CHECK(22.0f == temp().value().InCelsius().value());
        BOOST_REQUIRE(temp.TemperatureArea().has_value());
        BOOST_CHECK("Pool" == *(temp.TemperatureArea()));
    }

    {
        TemperatureStringConverter temp("Air         18`C");
        BOOST_REQUIRE(temp().has_value());
        BOOST_CHECK(18.0f == temp().value().InCelsius().value());
        BOOST_REQUIRE(temp.TemperatureArea().has_value());
        BOOST_CHECK("Air" == *(temp.TemperatureArea()));
    }

    {
        TemperatureStringConverter temp("Air        -18`C");
        BOOST_REQUIRE(temp().has_value());
        BOOST_CHECK(-18.0f == temp().value().InCelsius().value());
        BOOST_REQUIRE(temp.TemperatureArea().has_value());
        BOOST_CHECK("Air" == *(temp.TemperatureArea()));
    }

    {
        TemperatureStringConverter temp("Pool         0`C");
        BOOST_REQUIRE(temp().has_value());
        BOOST_CHECK(0.0f == temp().value().InCelsius().value());
        BOOST_REQUIRE(temp.TemperatureArea().has_value());
        BOOST_CHECK("Pool" == *(temp.TemperatureArea()));
    }
}

BOOST_AUTO_TEST_CASE(Test_Temperature_InvalidTemperatureStrings)
{
    {
        TemperatureStringConverter temp("Pool22`C");
        BOOST_CHECK(!temp().has_value());
        BOOST_CHECK(!temp.TemperatureArea().has_value());
    }

    {
        TemperatureStringConverter temp("Pool 22`C extra");
        BOOST_CHECK(!temp().has_value());
        BOOST_CHECK(!temp.TemperatureArea().has_value());
    }

    {
        TemperatureStringConverter temp("Pool 22C");
        BOOST_CHECK(!temp().has_value());
        BOOST_CHECK(!temp.TemperatureArea().has_value());
    }

    {
        TemperatureStringConverter temp("22`C");
        BOOST_CHECK(!temp().has_value());
        BOOST_CHECK(!temp.TemperatureArea().has_value());
    }

    {
        TemperatureStringConverter temp("Pool `C");
        BOOST_CHECK(!temp().has_value());
        BOOST_CHECK(!temp.TemperatureArea().has_value());
    }

    {
        TemperatureStringConverter temp("Pool 22`");
        BOOST_CHECK(!temp().has_value());
        BOOST_CHECK(!temp.TemperatureArea().has_value());
    }

    {
        TemperatureStringConverter temp("Pool 22");
        BOOST_CHECK(!temp().has_value());
        BOOST_CHECK(!temp.TemperatureArea().has_value());
    }

    {
        TemperatureStringConverter temp("Pool          22`C");
        BOOST_CHECK(!temp().has_value());
        BOOST_CHECK(!temp.TemperatureArea().has_value());
    }
}

BOOST_AUTO_TEST_CASE(OstreamOperatorNormalCase)
{
}

BOOST_AUTO_TEST_CASE(FormatNormalCase)
{
    TemperatureStringConverter temp0("pool 22`C");
    BOOST_CHECK_EQUAL(std::format("{}", temp0), "TEMP=22\u00B0C");

    TemperatureStringConverter temp1("pool 22`C");
    BOOST_CHECK_EQUAL(std::format("{:C}", temp1), "TEMP=22\u00B0C");
   
    TemperatureStringConverter temp2("pool 22`C");
    BOOST_CHECK_EQUAL(std::format("{:F}", temp2), "TEMP=72\u00B0F");

    TemperatureStringConverter temp3("air 72`F");
    BOOST_CHECK_EQUAL(std::format("{:F}", temp3), "TEMP=72\u00B0F");

    ///FIXME - Add precision support to the std::formatter<Kernel::Temperature>
    
    //TemperatureStringConverter temp4("air 72`F");
    //BOOST_CHECK(std::format("{:.1C}", temp4) == "22.0\u00B0F");

    //TemperatureStringConverter temp5("pool 22`C");
    //BOOST_CHECK(std::format("{:.2C}", temp5) == "22.00\u00B0C");
}

BOOST_AUTO_TEST_SUITE_END();
