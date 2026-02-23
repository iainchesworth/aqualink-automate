#include <array>
#include <cstdint>
#include <format>
#include <span>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "formatters/array_standard_formatter.h"
#include "formatters/circular_buffer_standard_formatter.h"
#include "formatters/orp_formatter.h"
#include "formatters/ph_formatter.h"
#include "formatters/temperature_formatter.h"
#include "formatters/units_dimensionless_formatter.h"
#include "formatters/units_electric_potential_formatter.h"
#include "kernel/orp.h"
#include "kernel/ph.h"
#include "kernel/temperature.h"

using namespace AqualinkAutomate;

BOOST_AUTO_TEST_SUITE(FormatterTests)

// --- Temperature formatter ---

BOOST_AUTO_TEST_CASE(Test_Temperature_DefaultFormat)
{
	auto temp = Kernel::Temperature::ConvertToTemperatureInCelsius(25.0);
	BOOST_CHECK_EQUAL(std::format("{}", temp), "25\u00B0C");
}

BOOST_AUTO_TEST_CASE(Test_Temperature_CelsiusWithPrecision)
{
	auto temp = Kernel::Temperature::ConvertToTemperatureInCelsius(25.5);
	BOOST_CHECK_EQUAL(std::format("{:.1C}", temp), "25.5\u00B0C");
	BOOST_CHECK_EQUAL(std::format("{:.2C}", temp), "25.50\u00B0C");
}

BOOST_AUTO_TEST_CASE(Test_Temperature_FahrenheitFormat)
{
	auto temp = Kernel::Temperature::ConvertToTemperatureInCelsius(100.0);
	BOOST_CHECK_EQUAL(std::format("{:F}", temp), "212\u00B0F");
}

// --- pH formatter ---

BOOST_AUTO_TEST_CASE(Test_pH_DefaultFormat)
{
	Kernel::pH ph(7.35f);
	BOOST_CHECK_EQUAL(std::format("{}", ph), "7.4");
}

BOOST_AUTO_TEST_CASE(Test_pH_WholeNumber)
{
	Kernel::pH ph(8.0f);
	BOOST_CHECK_EQUAL(std::format("{}", ph), "8.0");
}

// --- ORP formatter (delegates to millivolt_quantity) ---

BOOST_AUTO_TEST_CASE(Test_ORP_DefaultFormat)
{
	Kernel::ORP orp(650);
	BOOST_CHECK_EQUAL(std::format("{}", orp), "650mV");
}

// --- Units: ppm ---

BOOST_AUTO_TEST_CASE(Test_PPM_DefaultFormat)
{
	Units::ppm_quantity val = 3200.0 * Units::ppm;
	BOOST_CHECK_EQUAL(std::format("{}", val), "3200 ppm");
}

// --- Units: millivolt ---

BOOST_AUTO_TEST_CASE(Test_Millivolt_DefaultFormat)
{
	Units::millivolt_quantity val = 500.0 * Units::millivolt;
	BOOST_CHECK_EQUAL(std::format("{}", val), "500mV");
}

// --- Units: volt ---

BOOST_AUTO_TEST_CASE(Test_Volt_DefaultFormat)
{
	Units::volt_quantity val = 12.0 * Units::volt;
	BOOST_CHECK_EQUAL(std::format("{}", val), "12V");
}

// --- Hex byte formatters: no trailing space ---

BOOST_AUTO_TEST_CASE(Test_VectorUint8_Format_NoTrailingSpace)
{
	std::vector<uint8_t> data = { 0x10, 0x02, 0xff };
	auto result = std::format("{}", data);
	BOOST_CHECK_EQUAL(result, "0x10 0x02 0xff");
}

BOOST_AUTO_TEST_CASE(Test_VectorUint8_Format_Empty)
{
	std::vector<uint8_t> data = {};
	auto result = std::format("{}", data);
	BOOST_CHECK_EQUAL(result, "");
}

BOOST_AUTO_TEST_CASE(Test_VectorUint8_Format_SingleByte)
{
	std::vector<uint8_t> data = { 0xab };
	auto result = std::format("{}", data);
	BOOST_CHECK_EQUAL(result, "0xab");
}

BOOST_AUTO_TEST_CASE(Test_ArrayUint8_16_Format_NoTrailingSpace)
{
	std::array<uint8_t, 16> data = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	                                  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
	auto result = std::format("{}", data);
	// Should not have trailing space
	BOOST_CHECK(result.back() != ' ');
	BOOST_CHECK_EQUAL(result, "0x00 0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08 0x09 0x0a 0x0b 0x0c 0x0d 0x0e 0x0f");
}

BOOST_AUTO_TEST_CASE(Test_SpanUint8_Format_NoTrailingSpace)
{
	std::vector<uint8_t> backing = { 0xde, 0xad, 0xbe, 0xef };
	std::span<uint8_t> data(backing);
	auto result = std::format("{}", data);
	BOOST_CHECK_EQUAL(result, "0xde 0xad 0xbe 0xef");
}

// --- Circular buffer formatter ---

BOOST_AUTO_TEST_CASE(Test_CircularBuffer_StatsMode)
{
	boost::circular_buffer<uint8_t> buf(16);
	buf.push_back(0x10);
	buf.push_back(0x02);
	auto result = std::format("{}", buf);
	// Should contain stats like size= and capacity=
	BOOST_CHECK(result.find("size=2") != std::string::npos);
	BOOST_CHECK(result.find("capacity=16") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(Test_CircularBuffer_HexMode)
{
	boost::circular_buffer<uint8_t> buf(16);
	buf.push_back(0xaa);
	buf.push_back(0xbb);
	buf.push_back(0xcc);
	auto result = std::format("{:.}", buf);
	BOOST_CHECK_EQUAL(result, "0xaa 0xbb 0xcc");
}

BOOST_AUTO_TEST_CASE(Test_CircularBuffer_HexModeWithPrecision)
{
	boost::circular_buffer<uint8_t> buf(16);
	buf.push_back(0x01);
	buf.push_back(0x02);
	buf.push_back(0x03);
	buf.push_back(0x04);
	auto result = std::format("{:.2}", buf);
	BOOST_CHECK_EQUAL(result, "0x01 0x02");
}

BOOST_AUTO_TEST_SUITE_END()
