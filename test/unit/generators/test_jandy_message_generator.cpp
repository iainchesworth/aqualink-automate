#include <cstdint>
#include <random>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "jandy/errors/jandy_errors_messages.h"
#include "jandy/errors/jandy_errors_protocol.h"
#include "jandy/formatters/jandy_message_formatters.h"
#include "jandy/generator/jandy_message_generator.h"
#include "jandy/types/jandy_types.h"
#include "logging/logging.h"

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
};

BOOST_FIXTURE_TEST_SUITE(JandyMessageGenerator, JandyMessageGenerator_TestFixture);

BOOST_AUTO_TEST_CASE(ZeroDataInSerialBuffer)
{
	std::vector<uint8_t> test_data = { };

	QueueTest(test_data, Test_WaitingForMoreData, "Test Iteration - WAITING FOR DATA");
	StopTests(test_data, 0, "STOPPING TEST");

	RunTests();
}

BOOST_AUTO_TEST_CASE(SingleByteInSerialBuffer)
{
	std::vector<uint8_t> test_data = { 0x10 };

	QueueTest(test_data, Test_WaitingForMoreData, "Test Iteration - WAITING FOR DATA");
	StopTests(test_data, 0, "STOPPING TEST");

	RunTests();
}

BOOST_AUTO_TEST_CASE(InvalidDataInSerialBuffer)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(32, 127);
	std::vector<uint8_t> test_data;

	test_data.reserve(255);

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
	std::vector<uint8_t> test_data =
	{
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
	};

	QueueTest(test_data, Test_DataAvailableToProcess, "Test Iteration - WAITING FOR DATA");
	QueueTest(test_data, Test_DataAvailableToProcess, "Test Iteration - WAITING FOR DATA");
	QueueTest(test_data, Test_DataAvailableToProcess, "Test Iteration - WAITING FOR DATA");
	QueueTest(test_data, Test_DataAvailableToProcess, "Test Iteration - WAITING FOR DATA");
	QueueTest(test_data, Test_DataAvailableToProcess, "Test Iteration - WAITING FOR DATA");
	QueueTest(test_data, Test_DataAvailableToProcess, "Test Iteration - WAITING FOR DATA");
	QueueTest(test_data, Test_DataAvailableToProcess, "Test Iteration - WAITING FOR DATA");
	QueueTest(test_data, Test_WaitingForMoreData, "Test Iteration - WAITING FOR DATA");
	StopTests(test_data, 0, "STOPPING TEST");

	RunTests();

}

BOOST_AUTO_TEST_CASE(PacketStartsButIsIncomplete)
{
	std::vector<uint8_t> test_data =
	{
		0x10, 0x02, 0x40, 0x02, 0x00, 0x00, 0x00, 0x40, 0x04, 0x00, 0x00, 0x00,  // <-- Packet is missing end byte sequence
		0x10, 0x02, 0x00, 0x01, 0x80, 0x00, 0x93, 0x10, 0x03,
		0x10, 0x02, 0x80, 0x00, 0x92, 0x10, 0x03,

		0x10, 0x02, 0x40, 0x02, 0x00, 0x00, 0x00, 0x40, 0x04, 0x00, 0x00, 0x00,  // <-- Packet is missing end byte sequence
		0x10, 0x02, 0x00, 0x01, 0x80, 0x00, 0x93, 0x10, 0x03,
		0x10, 0x02, 0x80, 0x00, 0x92, 0x10, 0x03
	};

	QueueTest(test_data, Test_DataAvailableToProcess, "Test Iteration - DATA AVAILABLE");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 02");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 02");
	QueueTest(test_data, Test_DataAvailableToProcess, "Test Iteration - DATA AVAILABLE");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 03");
	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 04");
	StopTests(test_data, 0, "STOPPING TEST");

	RunTests();
}

BOOST_AUTO_TEST_CASE(FollowingPacketStartsButIsIncomplete)
{
	std::vector<uint8_t> test_data =
	{
		0x10, 0x02, 0x40, 0x02, 0x00, 0x00, 0x00, 0x40, 0x04, 0x98, 0x10, 0x03, 0x10, 0x02, 0x00, 0x01,
		0x80, 0x00
	};

	QueueTest(test_data, Test_ValidMessageOfAnyType, "Test Iteration - MESSAGE 01");
	QueueTest(test_data, Test_WaitingForMoreData, "Test Iteration - WAITING FOR DATA");
	StopTests(test_data, 6, "STOPPING TEST"); // <-- there's 6 bytes left as the processor is "waiting for data"...

	RunTests();
}

BOOST_AUTO_TEST_CASE(PacketStartsButLongerThanMaximumLength)
{
	std::vector<uint8_t> test_data =
	{
		0x10, 0x02, 0x40, 0x02, 0x00, 0x00, 0x00, 0x40, 0x04, 0x98, 0x10, 0xFF, 0x10, 0xFF, 0x00, 0x01,
		0x80, 0x00, 0x93, 0x10, 0xFF, 0x10, 0xFF, 0x80, 0x00, 0x92, 0x10, 0xFF, 0x10, 0xFF, 0x10, 0x00,
		0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x10, 0xFF, 0x00, 0x10, 0xFF, 0x00, 0x01, 0x00, 0x00,
		0x13, 0x10, 0xFF, 0x10, 0xFF, 0x33, 0x30, 0x75, 0x10, 0xFF, 0x00, 0x00, 0x10, 0xFF, 0x00, 0x01,
		0x00, 0x00, 0x13, 0x10, 0xFF, 0x10, 0xFF, 0xa3, 0x53, 0x08, 0x10, 0xFF, 0x00, 0x00, 0x10, 0xFF,
		0x00, 0x01, 0x3f, 0x00, 0x52, 0x10, 0xFF, 0x10, 0xFF, 0x50, 0x11, 0x0a, 0x7d, 0x10, 0xFF, 0x00,
		0x10, 0xFF, 0x00, 0x16, 0x28, 0x00, 0x00, 0x00, 0x50, 0x10, 0xFF, 0x10, 0xFF, 0x60, 0x00, 0x72,
		0x10, 0xFF, 0x00, 0x16, 0x28, 0x00, 0x00, 0x00, 0x50, 0x10, 0xFF, 0x10, 0xFF, 0x60, 0x00, 0x72,
		0x10, 0xFF, 0x10, 0xFF, 0x40, 0x02, 0x00, 0x00, 0x00, 0x40, 0x04, 0x98, 0x10, 0xFF, 0x10, 0x03   // <-- 144 byte payload is here...invalid checksum
	};


	QueueTest(test_data, Test_WaitingForMoreData, "Test Iteration - WAITING FOR DATA");
	StopTests(test_data, 0, "STOPPING TEST");

	RunTests();
}

BOOST_AUTO_TEST_CASE(ValidPackets_50)
{
	std::vector<uint8_t> test_data =
	{
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
	};

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

BOOST_AUTO_TEST_SUITE_END();
