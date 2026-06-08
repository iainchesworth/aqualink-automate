#include <cstddef>
#include <cstdint>
#include <random>

#include <boost/circular_buffer.hpp>
#include <boost/test/unit_test.hpp>

#include "errors/message_errors.h"
#include "errors/protocol_errors.h"
#include "jandy/formatters/jandy_message_formatters.h"
#include "jandy/generator/jandy_message_generator.h"
#include "jandy/generator/jandy_message_generator_packetvalidation.h"
#include "jandy/messages/jandy_message.h"
#include "jandy/types/jandy_types.h"
#include "logging/logging.h"

#include "support/unit_test_circularbuffermaker.h"
#include "support/unit_test_ostream_support.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::ErrorCodes;
using namespace AqualinkAutomate::ErrorCodes::Protocol;
using namespace AqualinkAutomate::Generators;
using namespace AqualinkAutomate::Logging;
using namespace AqualinkAutomate::Messages;

class JandyMessageGenerator_TestFixture
{
public:

	JandyMessageGenerator_TestFixture()
	{
		BOOST_TEST_MESSAGE("Setup: JandyMessageGenerator Test Fixure");

		Test_DataAvailableToProcess = [&](const TestReturnType& result) -> void
		{
			if (!result.has_value())
			{
				BOOST_TEST(make_error_code(ErrorCodes::Protocol_ErrorCodes::DataAvailableToProcess) == result.error());
			}
			else
			{
				BOOST_TEST(!result.has_value());
			}
		};

		Test_ValidMessageOfAnyType = [&](const TestReturnType& result) -> void
		{
			BOOST_TEST(result.has_value());
			if (result.has_value())
			{
				auto message = result.value();
				BOOST_TEST(nullptr != message);
			}
		};

		Test_WaitingForMoreData = [&](const TestReturnType& result) -> void
		{
			if (!result.has_value())
			{
				BOOST_TEST(make_error_code(ErrorCodes::Protocol_ErrorCodes::WaitingForMoreData) == result.error());
			}
			else
			{
				BOOST_TEST(!result.has_value());
			}
		};

		Test_ChecksumFailure = [&](const TestReturnType& result) -> void
		{
			if (!result.has_value())
			{
				BOOST_TEST(make_error_code(ErrorCodes::Protocol_ErrorCodes::ChecksumFailure) == result.error());
			}
			else
			{
				BOOST_TEST(!result.has_value());
			}
		};

		Test_OverlappingPackets = [&](const TestReturnType& result) -> void
		{
			if (!result.has_value())
			{
				BOOST_TEST(make_error_code(ErrorCodes::Protocol_ErrorCodes::OverlappingPackets) == result.error());
			}
			else
			{
				BOOST_TEST(!result.has_value());
			}
		};
	};

	~JandyMessageGenerator_TestFixture()
	{
		BOOST_TEST_MESSAGE("Teardown: JandyMessageGenerator Test Fixure");
	}

public:
	void QueueTest(auto& test_data, auto& result_checker, auto& message_to_print)
	{
		LogDebug(Channel::Main, message_to_print);
		auto result = Generators::GenerateMessageFromRawData(test_data);
		result_checker(result);
	};

public:
	void StopTests(auto& test_data, uint32_t remaining_bytes, auto& message_to_print)
	{
		LogDebug(Channel::Main, message_to_print);
		BOOST_TEST(remaining_bytes == test_data.size());
	}

public:
	void RunTests()
	{
	}

public:
	using TestReturnType = Types::JandyExpectedMessageType;
	std::function<void(const TestReturnType& result)> Test_DataAvailableToProcess;
	std::function<void(const TestReturnType& result)> Test_ValidMessageOfAnyType;
	std::function<void(const TestReturnType& result)> Test_WaitingForMoreData;
	std::function<void(const TestReturnType& result)> Test_ChecksumFailure;
	std::function<void(const TestReturnType& result)> Test_OverlappingPackets;
};

BOOST_FIXTURE_TEST_SUITE(JandyMessageGenerator, JandyMessageGenerator_TestFixture);

BOOST_AUTO_TEST_CASE(ZeroDataInSerialBuffer)
{
	auto test_data = Test::MakeCircularBuffer<uint8_t>({ });

	QueueTest(test_data, Test_WaitingForMoreData, "Test Iteration - WAITING FOR DATA");
	StopTests(test_data, 0, "STOPPING TEST");

	RunTests();
}

BOOST_AUTO_TEST_CASE(SingleByteInSerialBuffer)
{
	auto test_data = Test::MakeCircularBuffer<uint8_t>({ 0x10 });

	QueueTest(test_data, Test_WaitingForMoreData, "Test Iteration - WAITING FOR DATA");
	StopTests(test_data, 0, "STOPPING TEST");

	RunTests();
}

BOOST_AUTO_TEST_CASE(InvalidDataInSerialBuffer)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(32, 127);
	boost::circular_buffer<uint8_t> test_data(255);

	for (auto& elem : test_data)
	{
		elem = dis(gen);
	}

	QueueTest(test_data, Test_WaitingForMoreData, "Test Iteration - WAITING FOR DATA");
	StopTests(test_data, 0, "STOPPING TEST");

	RunTests();
}

BOOST_AUTO_TEST_CASE(InvalidMessageTypesInSerialData)
{
}

BOOST_AUTO_TEST_CASE(InvalidPacketChecksumsInSerialData)
{
	auto test_data = Test::MakeCircularBuffer<uint8_t>({
		0x10, 0x02, 0x40, 0x02, 0x00, 0x00, 0x00, 0x40, 0x04, 0x00, 0x10, 0x03,
		0x10, 0x02, 0x00, 0x01, 0x80, 0x00, 0x00, 0x10, 0x03,
		0x10, 0x02, 0x80, 0x00, 0x00, 0x10, 0x03,
		0x10, 0x02, 0x10, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x03,
		0x00, // <-- Random NUL byte (in recorded data)
		0x10, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x03,
		0x10, 0x02, 0x33, 0x30, 0x00, 0x10, 0x03,
		0x00, 0x00,  // <-- Random NUL bytes (in recorded data)
		0x10, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x03,
		0x10, 0x02, 0xa3, 0x53, 0x00, 0x10, 0x03
	});

	QueueTest(test_data, Test_ChecksumFailure, "Test Iteration - CHECKSUM FAILURE");
	QueueTest(test_data, Test_ChecksumFailure, "Test Iteration - CHECKSUM FAILURE");
	QueueTest(test_data, Test_ChecksumFailure, "Test Iteration - CHECKSUM FAILURE");
	QueueTest(test_data, Test_ChecksumFailure, "Test Iteration - CHECKSUM FAILURE");
	QueueTest(test_data, Test_ChecksumFailure, "Test Iteration - CHECKSUM FAILURE");
	QueueTest(test_data, Test_ChecksumFailure, "Test Iteration - CHECKSUM FAILURE");
	QueueTest(test_data, Test_ChecksumFailure, "Test Iteration - CHECKSUM FAILURE");
	QueueTest(test_data, Test_ChecksumFailure, "Test Iteration - CHECKSUM FAILURE");
	StopTests(test_data, 0, "STOPPING TEST");

	RunTests();

}

BOOST_AUTO_TEST_CASE(PacketStartsButIsIncomplete)
{
	auto test_data = Test::MakeCircularBuffer<uint8_t>({
		0x10, 0x02, 0x40, 0x02, 0x00, 0x00, 0x00, 0x40, 0x04, 0x00, 0x00, 0x00,  // <-- Packet is missing end byte sequence
		0x10, 0x02, 0x00, 0x01, 0x80, 0x00, 0x93, 0x10, 0x03,
		0x10, 0x02, 0x80, 0x00, 0x92, 0x10, 0x03,

		0x10, 0x02, 0x40, 0x02, 0x00, 0x00, 0x00, 0x40, 0x04, 0x00, 0x00, 0x00,  // <-- Packet is missing end byte sequence
		0x10, 0x02, 0x00, 0x01, 0x80, 0x00, 0x93, 0x10, 0x03,
		0x10, 0x02, 0x80, 0x00, 0x92, 0x10, 0x03
	});

	QueueTest(test_data, Test_OverlappingPackets, "Test Iteration - OVERLAPPING PACKETS");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 02");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 02");
	QueueTest(test_data, Test_OverlappingPackets, "Test Iteration - OVERLAPPING PACKETS");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 03");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 04");
	StopTests(test_data, 0, "STOPPING TEST");

	RunTests();
}

BOOST_AUTO_TEST_CASE(FollowingPacketStartsButIsIncomplete)
{
	auto test_data = Test::MakeCircularBuffer<uint8_t>({
		0x10, 0x02, 0x40, 0x02, 0x00, 0x00, 0x00, 0x40, 0x04, 0x98, 0x10, 0x03, 0x10, 0x02, 0x00, 0x01,
		0x80, 0x00
	});

	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 01");
	QueueTest(test_data, Test_WaitingForMoreData, "Test Iteration - WAITING FOR DATA");
	StopTests(test_data, 6, "STOPPING TEST"); // <-- there's 6 bytes left as the processor is "waiting for data"...

	RunTests();
}

BOOST_AUTO_TEST_CASE(PacketStartsButLongerThanMaximumLength)
{
	auto test_data = Test::MakeCircularBuffer<uint8_t>({
		0x10, 0x02, 0x40, 0x02, 0x00, 0x00, 0x00, 0x40, 0x04, 0x98, 0x10, 0xFF, 0x10, 0xFF, 0x00, 0x01,
		0x80, 0x00, 0x93, 0x10, 0xFF, 0x10, 0xFF, 0x80, 0x00, 0x92, 0x10, 0xFF, 0x10, 0xFF, 0x10, 0x00,
		0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x10, 0xFF, 0x00, 0x10, 0xFF, 0x00, 0x01, 0x00, 0x00,
		0x13, 0x10, 0xFF, 0x10, 0xFF, 0x33, 0x30, 0x75, 0x10, 0xFF, 0x00, 0x00, 0x10, 0xFF, 0x00, 0x01,
		0x00, 0x00, 0x13, 0x10, 0xFF, 0x10, 0xFF, 0xa3, 0x53, 0x08, 0x10, 0xFF, 0x00, 0x00, 0x10, 0xFF,
		0x00, 0x01, 0x3f, 0x00, 0x52, 0x10, 0xFF, 0x10, 0xFF, 0x50, 0x11, 0x0a, 0x7d, 0x10, 0xFF, 0x00,
		0x10, 0xFF, 0x00, 0x16, 0x28, 0x00, 0x00, 0x00, 0x50, 0x10, 0xFF, 0x10, 0xFF, 0x60, 0x00, 0x72,
		0x10, 0xFF, 0x00, 0x16, 0x28, 0x00, 0x00, 0x00, 0x50, 0x10, 0xFF, 0x10, 0xFF, 0x60, 0x00, 0x72,
		0x10, 0xFF, 0x10, 0xFF, 0x40, 0x02, 0x00, 0x00, 0x00, 0x40, 0x04, 0x98, 0x10, 0xFF, 0x10, 0x03   // <-- 144 byte payload is here...invalid checksum
	});

	QueueTest(test_data, Test_WaitingForMoreData, "Test Iteration - WAITING FOR DATA");
	StopTests(test_data, 0, "STOPPING TEST");

	RunTests();
}

BOOST_AUTO_TEST_CASE(PacketStartsButLongerThanMaximumLengthWithNoEndSequence)
{
	// Regression: BufferCleanUp_HasEndOfPacketWithinMaxDistance used to advance the erase endpoint by
	// PACKET_END_SEQUENCE.size() unconditionally. When a packet start is present but NO end sequence has
	// been received, p1_end == serial_data.end(); adding 2 pushed the erase endpoint past end() (an
	// out-of-bounds iterator) once the buffer exceeded MAXIMUM_PACKET_LENGTH bytes. This must now be
	// handled cleanly by clamping the erase endpoint to end().

	boost::circular_buffer<uint8_t> test_data(200);
	test_data.push_back(0x10); // DLE
	test_data.push_back(0x02); // STX -> a valid packet start...
	for (std::size_t i = test_data.size(); i < 200; ++i)
	{
		// ...followed by filler that never forms a DLE/ETX end sequence nor a second DLE/STX start.
		test_data.push_back(0xAA);
	}

	BOOST_TEST(test_data.size() > static_cast<std::size_t>(JandyMessage::MAXIMUM_PACKET_LENGTH));

	QueueTest(test_data, Test_WaitingForMoreData, "Test Iteration - WAITING FOR DATA");
	StopTests(test_data, 0, "STOPPING TEST");

	RunTests();
}

BOOST_AUTO_TEST_CASE(ValidPackets_50)
{
	auto test_data = Test::MakeCircularBuffer<uint8_t>({
		0x10, 0x02, 0x40, 0x02, 0x00, 0x00, 0x00, 0x40, 0x04, 0x98, 0x10, 0x03, 0x10, 0x02, 0x00, 0x01,
		0x80, 0x00, 0x93, 0x10, 0x03, 0x10, 0x02, 0x80, 0x00, 0x92, 0x10, 0x03, 0x10, 0x02, 0x10, 0x00,
		0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x10, 0x03, 0x00, 0x10, 0x02, 0x00, 0x01, 0x00, 0x00,
		0x13, 0x10, 0x03, 0x10, 0x02, 0x33, 0x30, 0x75, 0x10, 0x03, 0x00, 0x00, 0x10, 0x02, 0x00, 0x01,
		0x00, 0x00, 0x13, 0x10, 0x03, 0x10, 0x02, 0xa3, 0x53, 0x08, 0x10, 0x03, 0x00, 0x00, 0x10, 0x02,
		0x00, 0x01, 0x3f, 0x00, 0x52, 0x10, 0x03, 0x10, 0x02, 0x50, 0x11, 0x0a, 0x7d, 0x10, 0x03, 0x00,
		0x10, 0x02, 0x00, 0x16, 0x28, 0x00, 0x00, 0x00, 0x50, 0x10, 0x03, 0x10, 0x02, 0x60, 0x00, 0x72,
		0x10, 0x03, 0x10, 0x02, 0x40, 0x02, 0x00, 0x00, 0x00, 0x40, 0x04, 0x98, 0x10, 0x03, 0x10, 0x02,
		0x00, 0x01, 0x80, 0x00, 0x93, 0x10, 0x03, 0x10, 0x02, 0x10, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x24, 0x10, 0x03, 0x00, 0x10, 0x02, 0x00, 0x01, 0x00, 0x00, 0x13, 0x10, 0x03, 0x10, 0x02,
		0x33, 0x30, 0x75, 0x10, 0x03, 0x00, 0x00, 0x10, 0x02, 0x00, 0x01, 0x00, 0x00, 0x13, 0x10, 0x03,
		0x10, 0x02, 0xa3, 0x53, 0x08, 0x10, 0x03, 0x00, 0x00, 0x10, 0x02, 0x00, 0x01, 0x3f, 0x00, 0x52,
		0x10, 0x03, 0x10, 0x02, 0x50, 0x11, 0x0a, 0x7d, 0x10, 0x03, 0x00, 0x10, 0x02, 0x00, 0x16, 0x28,
		0x00, 0x00, 0x00, 0x50, 0x10, 0x03, 0x10, 0x02, 0x30, 0x00, 0x42, 0x10, 0x03, 0x10, 0x02, 0x40,
		0x02, 0x00, 0x00, 0x00, 0x40, 0x04, 0x98, 0x10, 0x03, 0x10, 0x02, 0x00, 0x01, 0x80, 0x00, 0x93,
		0x10, 0x03, 0x10, 0x02, 0x10, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x10, 0x03, 0x00,
		0x10, 0x02, 0x00, 0x01, 0x00, 0x00, 0x13, 0x10, 0x03, 0x10, 0x02, 0x33, 0x30, 0x75, 0x10, 0x03,
		0x00, 0x00, 0x10, 0x02, 0x00, 0x01, 0x00, 0x00, 0x13, 0x10, 0x03, 0x10, 0x02, 0xa3, 0x53, 0x08,
		0x10, 0x03, 0x00, 0x00, 0x10, 0x02, 0x00, 0x01, 0x3f, 0x00, 0x52, 0x10, 0x03, 0x10, 0x02, 0x50,
		0x11, 0x0a, 0x7d, 0x10, 0x03, 0x00, 0x10, 0x02, 0x00, 0x16, 0x28, 0x00, 0x00, 0x00, 0x50, 0x10,
		0x03, 0x10, 0x02, 0x60, 0x00, 0x72, 0x10, 0x03, 0x10, 0x02, 0x40, 0x02, 0x00, 0x00, 0x00, 0x40,
		0x04, 0x98, 0x10, 0x03, 0x10, 0x02, 0x00, 0x01, 0x80, 0x00, 0x93, 0x10, 0x03, 0x10, 0x02, 0x10,
		0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x10, 0x03, 0x00, 0x10, 0x02, 0x00, 0x01, 0x00,
		0x00, 0x13, 0x10, 0x03, 0x10, 0x02, 0x33, 0x30, 0x75, 0x10, 0x03, 0x00, 0x00, 0x10, 0x02, 0x00,
		0x01, 0x00, 0x00, 0x13, 0x10, 0x03, 0x10, 0x02, 0xa3, 0x53, 0x08, 0x10, 0x03, 0x00, 0x00, 0x10,
		0x02, 0x00, 0x01, 0x3f, 0x00, 0x52, 0x10, 0x03, 0x10, 0x02, 0x50, 0x11, 0x0a, 0x7d, 0x10, 0x03,
		0x00, 0x10, 0x02, 0x00, 0x16, 0x28, 0x00, 0x00, 0x00, 0x50, 0x10, 0x03, 0x10, 0x02, 0x31, 0x00,
		0x43, 0x10, 0x03, 0x10, 0x02, 0x40, 0x02, 0x00, 0x00, 0x00, 0x40, 0x04, 0x98, 0x10, 0x03, 0x10,
		0x02, 0x00, 0x01, 0x80, 0x00, 0x93, 0x10, 0x03, 0x10, 0x02, 0x10, 0x00, 0x02, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x24, 0x10, 0x03, 0x00, 0x10, 0x02, 0x00, 0x01, 0x00, 0x00, 0x13, 0x10, 0x03, 0x10,
		0x02, 0x81, 0x00, 0x93, 0x10, 0x03
	});

	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 01");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 02");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 03");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 04");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 05");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 06");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 07");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 09");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 10");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 11");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 13");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 14");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 15");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 16");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 17");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 18");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 19");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 20");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 21");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 22");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 23");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 24");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 25");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 26");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 27");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 28");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 29");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 30");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 31");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 32");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 33");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 34");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 35");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 36");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 37");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 38");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 39");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 40");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 41");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 42");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 43");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 44");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 45");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 46");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 47");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 48");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 49");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 50");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 51");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 52");
	QueueTest(test_data, Test_WaitingForMoreData, "Test Iteration - WAITING FOR DATA");
	StopTests(test_data, 0, "STOPPING TEST");

	RunTests();
}

BOOST_AUTO_TEST_CASE(ChecksumValidation_RejectsUnderLengthRange)
{
	// Regression: PacketValidation_ChecksumIsValid computes the checksum iterator at (length - 3) with no
	// lower-bound guard. A range shorter than 3 bytes would wrap the unsigned subtraction and form an
	// out-of-bounds iterator. A range of length < 3 must now be rejected outright.

	auto buffer_len_0 = Test::MakeCircularBuffer<uint8_t>({ });
	BOOST_TEST(false == PacketValidation_ChecksumIsValid(buffer_len_0.begin(), buffer_len_0.end()));

	auto buffer_len_1 = Test::MakeCircularBuffer<uint8_t>({ 0x10 });
	BOOST_TEST(false == PacketValidation_ChecksumIsValid(buffer_len_1.begin(), buffer_len_1.end()));

	auto buffer_len_2 = Test::MakeCircularBuffer<uint8_t>({ 0x10, 0x02 });
	BOOST_TEST(false == PacketValidation_ChecksumIsValid(buffer_len_2.begin(), buffer_len_2.end()));
}

BOOST_AUTO_TEST_CASE(ChecksumValidation_AcceptsValidMinimalChecksum)
{
	// A length-3 range is the smallest the validator accepts. The checksum byte sits at index (length - 3),
	// i.e. index 0, so the summed range [begin, checksum_it) is empty and the expected checksum is 0x00.
	// Confirm both a matching and a mismatching checksum are handled without going out of bounds.

	auto matching = Test::MakeCircularBuffer<uint8_t>({ 0x00, 0xAA, 0xBB });
	BOOST_TEST(true == PacketValidation_ChecksumIsValid(matching.begin(), matching.end()));

	auto mismatching = Test::MakeCircularBuffer<uint8_t>({ 0x01, 0xAA, 0xBB });
	BOOST_TEST(false == PacketValidation_ChecksumIsValid(mismatching.begin(), mismatching.end()));
}

BOOST_AUTO_TEST_SUITE_END();
