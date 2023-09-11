#include <boost/test/unit_test.hpp>

#include "jandy/utility/string_conversion/temperature.h"
#include "kernel/temperature.h"

#include "support/unit_test_ostream_support.h"

BOOST_AUTO_TEST_SUITE(TestSuite_Temperature);

BOOST_AUTO_TEST_CASE(Test_Temperature_ValidTemperatureStrings)
{
    using AqualinkAutomate::Utility::Temperature;

    {
        Temperature temp("pool 22`C");
        BOOST_REQUIRE(temp().has_value());
        BOOST_TEST(22.0f == temp().value().InCelsius().value());
        BOOST_REQUIRE(temp.TemperatureArea().has_value());
        BOOST_TEST("pool" == *(temp.TemperatureArea()));
    }

    {
        Temperature temp("air 72`F");
        BOOST_REQUIRE(temp().has_value());
        BOOST_TEST(72.0f == temp().value().InFahrenheit().value());
        BOOST_REQUIRE(temp.TemperatureArea().has_value());
        BOOST_TEST("air" == *(temp.TemperatureArea()));
    }

    {
        Temperature temp("Pool 22`C");
        BOOST_REQUIRE(temp().has_value());
        BOOST_TEST(22.0f == temp().value().InCelsius().value());
        BOOST_REQUIRE(temp.TemperatureArea().has_value());
        BOOST_TEST("Pool" == *(temp.TemperatureArea()));
    }

    {
        Temperature temp("Air 18`C");
        BOOST_REQUIRE(temp().has_value());
        BOOST_TEST(18.0f == temp().value().InCelsius().value());
        BOOST_REQUIRE(temp.TemperatureArea().has_value());
        BOOST_TEST("Air" == *(temp.TemperatureArea()));
    }

    {
        Temperature temp("Air -18`C");
        BOOST_REQUIRE(temp().has_value());
        BOOST_TEST(-18.0f == temp().value().InCelsius().value());
        BOOST_REQUIRE(temp.TemperatureArea().has_value());
        BOOST_TEST("Air" == *(temp.TemperatureArea()));
    }

    {
        Temperature temp("Pool 0`C");
        BOOST_REQUIRE(temp().has_value());
        BOOST_TEST(0.0f == temp().value().InCelsius().value());
        BOOST_REQUIRE(temp.TemperatureArea().has_value());
        BOOST_TEST("Pool" == *(temp.TemperatureArea()));
    }

    {
        Temperature temp("pool        22`C");
        BOOST_REQUIRE(temp().has_value());
        BOOST_TEST(22.0f == temp().value().InCelsius().value());
        BOOST_REQUIRE(temp.TemperatureArea().has_value());
        BOOST_TEST("pool" == *(temp.TemperatureArea()));
    }

    {
        Temperature temp("pool        72`F");
        BOOST_REQUIRE(temp().has_value());
        BOOST_TEST(72.0f == temp().value().InFahrenheit().value());
        BOOST_REQUIRE(temp.TemperatureArea().has_value());
        BOOST_TEST("pool" == *(temp.TemperatureArea()));
    }

    {
        Temperature temp("someth 18`C");
        BOOST_REQUIRE(temp().has_value());
        BOOST_TEST(18.0f == temp().value().InCelsius().value());
        BOOST_REQUIRE(temp.TemperatureArea().has_value());
        BOOST_TEST("someth" == *(temp.TemperatureArea()));
    }

    {
        Temperature temp("Pool        22`C");
        BOOST_REQUIRE(temp().has_value());
        BOOST_TEST(22.0f == temp().value().InCelsius().value());
        BOOST_REQUIRE(temp.TemperatureArea().has_value());
        BOOST_TEST("Pool" == *(temp.TemperatureArea()));
    }

    {
        Temperature temp("Air         18`C");
        BOOST_REQUIRE(temp().has_value());
        BOOST_TEST(18.0f == temp().value().InCelsius().value());
        BOOST_REQUIRE(temp.TemperatureArea().has_value());
        BOOST_TEST("Air" == *(temp.TemperatureArea()));
    }

    {
        Temperature temp("Air        -18`C");
        BOOST_REQUIRE(temp().has_value());
        BOOST_TEST(-18.0f == temp().value().InCelsius().value());
        BOOST_REQUIRE(temp.TemperatureArea().has_value());
        BOOST_TEST("Air" == *(temp.TemperatureArea()));
    }

    {
        Temperature temp("Pool         0`C");
        BOOST_REQUIRE(temp().has_value());
        BOOST_TEST(0.0f == temp().value().InCelsius().value());
        BOOST_REQUIRE(temp.TemperatureArea().has_value());
        BOOST_TEST("Pool" == *(temp.TemperatureArea()));
    }
}

BOOST_AUTO_TEST_CASE(Test_Temperature_InvalidTemperatureStrings)
{
    using AqualinkAutomate::Utility::Temperature;

    {
        Temperature temp("Pool22`C");
        BOOST_TEST(!temp().has_value());
        BOOST_TEST(!temp.TemperatureArea().has_value());
    }

    {
        Temperature temp("Pool 22`C extra");
        BOOST_TEST(!temp().has_value());
        BOOST_TEST(!temp.TemperatureArea().has_value());
    }

    {
        Temperature temp("Pool 22C");
        BOOST_TEST(!temp().has_value());
        BOOST_TEST(!temp.TemperatureArea().has_value());
    }

    {
        Temperature temp("22`C");
        BOOST_TEST(!temp().has_value());
        BOOST_TEST(!temp.TemperatureArea().has_value());
    }

    {
        Temperature temp("Pool `C");
        BOOST_TEST(!temp().has_value());
        BOOST_TEST(!temp.TemperatureArea().has_value());
    }

    {
        Temperature temp("Pool 22`");
        BOOST_TEST(!temp().has_value());
        BOOST_TEST(!temp.TemperatureArea().has_value());
    }

    {
        Temperature temp("Pool 22");
        BOOST_TEST(!temp().has_value());
        BOOST_TEST(!temp.TemperatureArea().has_value());
    }

    {
        Temperature temp("Pool          22`C");
        BOOST_TEST(!temp().has_value());
        BOOST_TEST(!temp.TemperatureArea().has_value());
    }
}

BOOST_AUTO_TEST_SUITE_END();
