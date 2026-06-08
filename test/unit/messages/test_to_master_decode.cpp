#include <span>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "jandy/equipment/master_traffic_snoop.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/jandy_message_unknown.h"

using namespace AqualinkAutomate;

BOOST_AUTO_TEST_SUITE(ToMasterTrafficDecodeTestSuite)

BOOST_AUTO_TEST_CASE(TestFormatToMasterTraffic_UnknownCommand_SurfacesCommandAndBytes)
{
	// An unrecognised command addressed TO the master (0x00) -- the exact case the
	// --decode-to-master diagnostic exists to surface, so commands an iAqualink2 (or any
	// other controller) sends to the master can be reverse engineered byte-for-byte.
	//   DLE STX 00 29  11 1c  <chk=0x68> DLE ETX   (chk = 0x10+0x02+0x00+0x29+0x11+0x1c)
	std::vector<uint8_t> frame = { 0x10, 0x02, 0x00, 0x29, 0x11, 0x1c, 0x68, 0x10, 0x03 };

	Messages::JandyMessage_Unknown message;
	BOOST_REQUIRE(message.Deserialize(std::as_bytes(std::span<uint8_t>(frame))));

	const std::string decoded = Equipment::FormatToMasterTraffic(message);

	// The frame must be flagged as master-bound and carry the real destination.
	BOOST_CHECK(decoded.find("[to-master]") != std::string::npos);
	BOOST_CHECK(decoded.find("AqualinkMaster (0x00)") != std::string::npos);
	// The actual wire command byte must be surfaced explicitly -- the message Id collapses
	// to the catch-all "Unknown" (0xff) and would otherwise hide the real command (0x29).
	BOOST_CHECK(decoded.find("cmd=0x29") != std::string::npos);
	// The raw payload bytes must be surfaced for byte-level analysis.
	BOOST_CHECK(decoded.find("11 1c") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()
