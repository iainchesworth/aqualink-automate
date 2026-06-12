#include <cstdint>
#include <span>
#include <vector>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/test/unit_test.hpp>

#include "serial/rfc2217/rfc2217_constants.h"
#include "serial/rfc2217/rfc2217_protocol_handler.h"

using namespace AqualinkAutomate::Serial::RFC2217;

namespace
{

	// Helper: build a handler bound to an (unconnected) socket.  The inbound byte
	// processor never touches the socket, so an unopened socket is sufficient to
	// exercise FilterInboundData / ProcessByte.
	struct HandlerFixture
	{
		boost::asio::io_context io_context;
		boost::asio::ip::tcp::socket socket{ io_context };
		ProtocolHandler handler{ socket };
	};

	std::vector<uint8_t> Filter(ProtocolHandler& handler, std::vector<uint8_t> raw)
	{
		const auto kept = handler.FilterInboundData(std::span<uint8_t>{ raw });
		raw.resize(kept);
		return raw;
	}

}
// anonymous namespace

BOOST_AUTO_TEST_SUITE(RFC2217InboundFilter_TestSuite)

// =============================================================================
// Plain data passes straight through.
// =============================================================================

BOOST_AUTO_TEST_CASE(PlainData_PassesThroughUnchanged)
{
	HandlerFixture fixture;

	const std::vector<uint8_t> expected{ 0x10, 0x02, 0x33, 0x00, 0x03 };
	const auto filtered = Filter(fixture.handler, expected);

	BOOST_CHECK_EQUAL_COLLECTIONS(filtered.begin(), filtered.end(), expected.begin(), expected.end());
}

// =============================================================================
// IAC IAC is a single escaped 0xFF data byte (0xFF is legal on the Jandy wire).
// =============================================================================

BOOST_AUTO_TEST_CASE(EscapedIAC_DecodesToSingleByte)
{
	HandlerFixture fixture;

	// 0x10 0x02 [IAC IAC -> 0xFF] 0x03
	const std::vector<uint8_t> raw{ 0x10, 0x02, Constants::IAC, Constants::IAC, 0x03 };
	const auto filtered = Filter(fixture.handler, raw);

	const std::vector<uint8_t> expected{ 0x10, 0x02, 0xFF, 0x03 };
	BOOST_CHECK_EQUAL_COLLECTIONS(filtered.begin(), filtered.end(), expected.begin(), expected.end());
}

// =============================================================================
// A bare IAC followed by a WILL/DO command consumes both bytes (no data leaks).
// =============================================================================

BOOST_AUTO_TEST_CASE(IACCommand_IsStrippedEntirely)
{
	HandlerFixture fixture;

	// 0xAA [IAC WILL COM_PORT_OPTION] 0xBB
	const std::vector<uint8_t> raw{ 0xAA, Constants::IAC, Constants::WILL, Constants::COM_PORT_OPTION, 0xBB };
	const auto filtered = Filter(fixture.handler, raw);

	const std::vector<uint8_t> expected{ 0xAA, 0xBB };
	BOOST_CHECK_EQUAL_COLLECTIONS(filtered.begin(), filtered.end(), expected.begin(), expected.end());
}

// =============================================================================
// A full subnegotiation block (IAC SB .. IAC SE) is consumed; surrounding data
// survives.
// =============================================================================

BOOST_AUTO_TEST_CASE(Subnegotiation_IsStrippedEntirely)
{
	HandlerFixture fixture;

	const std::vector<uint8_t> raw
	{
		0x01,
		Constants::IAC, Constants::SB,
		Constants::COM_PORT_OPTION, Constants::SET_BAUDRATE + Constants::SERVER_OFFSET,
		0x00, 0x00, 0x25, 0x80,           // 9600 baud, big-endian
		Constants::IAC, Constants::SE,
		0x02
	};
	const auto filtered = Filter(fixture.handler, raw);

	const std::vector<uint8_t> expected{ 0x01, 0x02 };
	BOOST_CHECK_EQUAL_COLLECTIONS(filtered.begin(), filtered.end(), expected.begin(), expected.end());
}

// =============================================================================
// An IAC byte inside a subnegotiation payload is unescaped (IAC IAC -> IAC) and
// the payload byte is retained within the consumed command, never emitted.
// =============================================================================

BOOST_AUTO_TEST_CASE(EscapedIACInsideSubnegotiation_DoesNotLeak)
{
	HandlerFixture fixture;

	const std::vector<uint8_t> raw
	{
		0x05,
		Constants::IAC, Constants::SB,
		Constants::COM_PORT_OPTION, Constants::SET_DATASIZE + Constants::SERVER_OFFSET,
		Constants::IAC, Constants::IAC,   // escaped 0xFF parameter byte
		Constants::IAC, Constants::SE,
		0x06
	};
	const auto filtered = Filter(fixture.handler, raw);

	const std::vector<uint8_t> expected{ 0x05, 0x06 };
	BOOST_CHECK_EQUAL_COLLECTIONS(filtered.begin(), filtered.end(), expected.begin(), expected.end());
}

// =============================================================================
// A subnegotiation split across two reads is decoded correctly because the
// handler retains state between calls (the real network read path delivers the
// stream in chunks).
// =============================================================================

BOOST_AUTO_TEST_CASE(Subnegotiation_SplitAcrossReads_RetainsState)
{
	HandlerFixture fixture;

	const auto chunk1 = Filter(fixture.handler,
		{ 0x07, Constants::IAC, Constants::SB, Constants::COM_PORT_OPTION });
	const auto chunk2 = Filter(fixture.handler,
		{ Constants::SET_PARITY + Constants::SERVER_OFFSET, 0x01, Constants::IAC, Constants::SE, 0x08 });

	const std::vector<uint8_t> expected1{ 0x07 };
	const std::vector<uint8_t> expected2{ 0x08 };
	BOOST_CHECK_EQUAL_COLLECTIONS(chunk1.begin(), chunk1.end(), expected1.begin(), expected1.end());
	BOOST_CHECK_EQUAL_COLLECTIONS(chunk2.begin(), chunk2.end(), expected2.begin(), expected2.end());
}

// =============================================================================
// FilterInboundData compacts in place: the returned length never exceeds the
// input length, and an all-control buffer yields zero data bytes.
// =============================================================================

BOOST_AUTO_TEST_CASE(AllControlBytes_YieldNoData)
{
	HandlerFixture fixture;

	const std::vector<uint8_t> raw
	{
		Constants::IAC, Constants::SB,
		Constants::COM_PORT_OPTION, Constants::SET_CONTROL + Constants::SERVER_OFFSET,
		0x00,
		Constants::IAC, Constants::SE
	};
	const auto filtered = Filter(fixture.handler, raw);

	BOOST_CHECK(filtered.empty());
}

BOOST_AUTO_TEST_SUITE_END()
