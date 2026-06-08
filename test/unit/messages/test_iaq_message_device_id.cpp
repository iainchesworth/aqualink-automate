#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "jandy/messages/jandy_message_constants.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/iaq/iaq_message_device_id.h"

using namespace AqualinkAutomate::Messages;

BOOST_AUTO_TEST_SUITE(IAQMessage_DeviceIdTestSuite)

BOOST_AUTO_TEST_CASE(TestConstruction)
{
	IAQMessage_DeviceId message;
	BOOST_CHECK(message.Id() == JandyMessageIds::IAQ_DeviceId);
	BOOST_CHECK(message.DeviceId().empty());
}

BOOST_AUTO_TEST_CASE(TestDeserialize_DecodesAsciiSerial)
{
	// Captured boot-init frame (test/fixtures/iaq_boot_sequence.cap): the master sends the
	// iAqualink2 (0xa3) its serial via cmd 0x51 as a NUL-terminated ASCII string.
	//   DLE STX a3 51  "1BA62825B7C69A4C" 00  <checksum> DLE ETX
	const std::vector<uint8_t> frame =
	{
		HEADER_BYTE_DLE, HEADER_BYTE_STX, 0xa3, 0x51,
		'1','B','A','6','2','8','2','5','B','7','C','6','9','A','4','C', 0x00,
		0xa4,
		HEADER_BYTE_DLE, HEADER_BYTE_ETX
	};

	IAQMessage_DeviceId message;
	BOOST_REQUIRE(message.DeserializeContents(frame));
	BOOST_CHECK_EQUAL(message.DeviceId(), std::string("1BA62825B7C69A4C"));
}

BOOST_AUTO_TEST_CASE(TestDeserialize_TooShort_ReturnsFalse)
{
	const std::vector<uint8_t> frame = { HEADER_BYTE_DLE, HEADER_BYTE_STX, 0xa3, 0x51, HEADER_BYTE_DLE, HEADER_BYTE_ETX };
	IAQMessage_DeviceId message;
	BOOST_CHECK_EQUAL(false, message.DeserializeContents(frame));
}

BOOST_AUTO_TEST_SUITE_END()
