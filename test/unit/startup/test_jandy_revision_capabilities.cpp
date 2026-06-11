#include <optional>
#include <string>

#include <boost/test/unit_test.hpp>

#include "jandy/startup/jandy_revision_capabilities.h"

using namespace AqualinkAutomate::Jandy::Startup;

BOOST_AUTO_TEST_SUITE(Jandy_Revision_Capabilities_TestSuite)

// =============================================================================
// ParseRevisionLetter -- extract the major revision from the reported string
// =============================================================================

BOOST_AUTO_TEST_CASE(ParseRevisionLetter_StripsRevPrefix)
{
	// "REV T.0.1" is the form the panel reports (seen on the PD-8 Combo); the leading "REV"
	// must be skipped so we get 'T', not the 'R' of "REV".
	BOOST_CHECK(ParseRevisionLetter("REV T.0.1") == std::optional<char>('T'));
	BOOST_CHECK(ParseRevisionLetter("Rev O") == std::optional<char>('O'));
	BOOST_CHECK(ParseRevisionLetter("  rev q ") == std::optional<char>('Q'));
}

BOOST_AUTO_TEST_CASE(ParseRevisionLetter_BareLetterOrVersion)
{
	BOOST_CHECK(ParseRevisionLetter("O") == std::optional<char>('O'));
	BOOST_CHECK(ParseRevisionLetter("o") == std::optional<char>('O'));
	BOOST_CHECK(ParseRevisionLetter("T.0.1") == std::optional<char>('T'));
}

BOOST_AUTO_TEST_CASE(ParseRevisionLetter_NoLetter_ReturnsNullopt)
{
	BOOST_CHECK(ParseRevisionLetter("") == std::nullopt);
	BOOST_CHECK(ParseRevisionLetter("REV") == std::nullopt);
	BOOST_CHECK(ParseRevisionLetter("1234") == std::nullopt);
}

// =============================================================================
// DeriveRevisionCapabilities -- the published feature gates
// =============================================================================

BOOST_AUTO_TEST_CASE(Caps_RevT_CpuTouchVsPumpsAndCloud)
{
	auto caps = DeriveRevisionCapabilities("REV T.0.1");
	BOOST_CHECK(caps.is_known);
	BOOST_CHECK_EQUAL(caps.revision_letter, 'T');
	BOOST_CHECK(caps.cpu_board);              // N+
	BOOST_CHECK(caps.variable_speed_pumps);   // O+
	BOOST_CHECK(caps.chemlink_chlorinators);  // P+
	BOOST_CHECK(caps.aqualink_touch);         // Q+
	BOOST_CHECK(caps.iaqualink_cloud);        // R+
	BOOST_CHECK(!caps.addressed_vs_pumps);    // W+
	BOOST_CHECK(!caps.infinite_watercolors);  // Y+
}

BOOST_AUTO_TEST_CASE(Caps_RevO_VsPumpsButNoChemlinkOrTouch)
{
	auto caps = DeriveRevisionCapabilities("Rev O");
	BOOST_CHECK(caps.cpu_board);
	BOOST_CHECK(caps.variable_speed_pumps);
	BOOST_CHECK(!caps.chemlink_chlorinators);  // P+
	BOOST_CHECK(!caps.aqualink_touch);         // Q+
}

BOOST_AUTO_TEST_CASE(Caps_RevM_PpdEraNoCpuFeatures)
{
	auto caps = DeriveRevisionCapabilities("M");
	BOOST_CHECK(caps.is_known);
	BOOST_CHECK(!caps.cpu_board);              // pre-N == PPD architecture
	BOOST_CHECK(!caps.variable_speed_pumps);
	BOOST_CHECK(!caps.aqualink_touch);
}

BOOST_AUTO_TEST_CASE(Caps_RevQ_TouchButNoCloud)
{
	auto caps = DeriveRevisionCapabilities("Q");
	BOOST_CHECK(caps.aqualink_touch);     // Q
	BOOST_CHECK(!caps.iaqualink_cloud);   // R+
}

BOOST_AUTO_TEST_CASE(Caps_RevW_AddressedVsPumps)
{
	auto caps = DeriveRevisionCapabilities("W");
	BOOST_CHECK(caps.addressed_vs_pumps);
	BOOST_CHECK(caps.aqualink_touch);         // all gates below W
	BOOST_CHECK(!caps.infinite_watercolors);  // Y+
}

BOOST_AUTO_TEST_CASE(Caps_Unknown_AllFalse)
{
	auto caps = DeriveRevisionCapabilities("");
	BOOST_CHECK(!caps.is_known);
	BOOST_CHECK(!caps.cpu_board);
	BOOST_CHECK(!caps.aqualink_touch);
}

BOOST_AUTO_TEST_SUITE_END()
