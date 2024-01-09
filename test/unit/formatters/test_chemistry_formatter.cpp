#include <sstream>

#include <boost/test/unit_test.hpp>

#include "jandy/formatters/chemistry_formatter.h"
#include "jandy/utility/string_conversion/chemistry.h"

BOOST_AUTO_TEST_SUITE(ChemistryFormatter)

BOOST_AUTO_TEST_CASE(OstreamOperatorNormalCase)
{
    AqualinkAutomate::Utility::Chemistry chem1("ORP/200 PH/6.8");

	std::ostringstream oss;
	oss << chem1;
	std::string result = oss.str();

	BOOST_CHECK_EQUAL(result, "ORP=200mV PH=6.8");
}

BOOST_AUTO_TEST_CASE(OstreamOperatorExceptionCase)
{
    AqualinkAutomate::Utility::Chemistry chem1("ORP/2");
    
    std::ostringstream oss;
    oss << chem1;
    std::string result = oss.str();

    BOOST_CHECK_EQUAL(result, "CHEM=??");
}

BOOST_AUTO_TEST_CASE(FormatNormalCase)
{
    AqualinkAutomate::Utility::Chemistry chem1("ORP/200 PH/6.8");
    AqualinkAutomate::Utility::Chemistry chem2("ORP/950 PH/8.2");

    {
        std::string result = std::format("{}", chem1);
        BOOST_CHECK_EQUAL(result, "ORP=200mV PH=6.8");
    }

    {
        std::string result = std::format("{}", chem2);
        BOOST_CHECK_EQUAL(result, "ORP=950mV PH=8.2");
    }
}

BOOST_AUTO_TEST_CASE(FormatExceptionCase)
{
    {
        AqualinkAutomate::Utility::Chemistry chem1("ORP/2");
        std::string result = std::format("{}", chem1);
        BOOST_CHECK_EQUAL(result, "CHEM=??");
    }

    {
        AqualinkAutomate::Utility::Chemistry chem2("ORP/10000 PH/10.00");
        std::string result = std::format("{}", chem2);
        BOOST_CHECK_EQUAL(result, "CHEM=??");
    }

    {
        AqualinkAutomate::Utility::Chemistry chem3("ORP/-300 PH/6.5");
        std::string result = std::format("{}", chem3);
        BOOST_CHECK_EQUAL(result, "CHEM=??");
    }

    {
        AqualinkAutomate::Utility::Chemistry chem4("ORP/ABC PH/DEF");
        std::string result = std::format("{}", chem4);
        BOOST_CHECK_EQUAL(result, "CHEM=??");
    }

    {
        AqualinkAutomate::Utility::Chemistry chem5("200 6.8");
        std::string result = std::format("{}", chem5);
        BOOST_CHECK_EQUAL(result, "CHEM=??");
    }
}

BOOST_AUTO_TEST_SUITE_END()
