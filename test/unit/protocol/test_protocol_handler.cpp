#include <cstdint>
#include <memory>
#include <vector>

#include <boost/asio/buffer.hpp>
#include <boost/test/unit_test.hpp>

#include "protocol/protocol_handler.h"
#include "serial/serial_port.h"

#include "mocks/mock_testserialportimpl.h"
#include "support/unit_test_protocolmessagebuilder.h"
#include "support/unit_test_hublocatorinjector.h"

using namespace AqualinkAutomate::Developer;
using namespace AqualinkAutomate::Protocol;

BOOST_AUTO_TEST_SUITE(ProtocolHandlerTestSuite)

struct ProtocolHandlerTestFixture
{
	ProtocolHandlerTestFixture() :
		hub_locator_injector(),
		test_serial_port_impl_ptr(new Test::TestSerialPortImpl()),
		test_serial_port_impl(*test_serial_port_impl_ptr),
		serial_port(std::unique_ptr<Test::TestSerialPortImpl>(test_serial_port_impl_ptr), hub_locator_injector)
	{
		test_serial_port_impl.EnableTestMode(true);
	}

	~ProtocolHandlerTestFixture()
	{
		test_serial_port_impl.Reset();
	}

	Test::HubLocatorInjector hub_locator_injector;
	Test::TestSerialPortImpl* test_serial_port_impl_ptr;
	Test::TestSerialPortImpl& test_serial_port_impl;
	Serial::SerialPort serial_port;
};

BOOST_FIXTURE_TEST_CASE(MockSerialPort_InitialState, ProtocolHandlerTestFixture)
{
	BOOST_TEST(test_serial_port_impl.is_open() == true);
	BOOST_TEST(test_serial_port_impl.GetReadCallCount() == 0);
	BOOST_TEST(test_serial_port_impl.GetWriteCallCount() == 0);
	BOOST_TEST(test_serial_port_impl.GetTotalBytesRead() == 0);
	BOOST_TEST(test_serial_port_impl.GetTotalBytesWritten() == 0);
}

BOOST_FIXTURE_TEST_CASE(MockSerialPort_QueueReadData, ProtocolHandlerTestFixture)
{
	std::vector<uint8_t> test_data = { 0x01, 0x02, 0x03, 0x04 };
	test_serial_port_impl.QueueReadData(test_data);

	BOOST_TEST(test_serial_port_impl.IsReadQueueEmpty() == false);
}

BOOST_FIXTURE_TEST_CASE(MockSerialPort_ReadSuccess, ProtocolHandlerTestFixture)
{
	std::vector<uint8_t> test_data = { 0x10, 0x02, 0x42, 0x03, 0x10, 0x03 };
	test_serial_port_impl.QueueReadData(test_data);

	std::array<uint8_t, 16> buffer{};
	boost::system::error_code ec;
	auto bytes_read = serial_port.read_some(boost::asio::buffer(buffer.data(), buffer.size()), ec);

	BOOST_TEST(!ec);
	BOOST_TEST(bytes_read == test_data.size());
	BOOST_TEST(test_serial_port_impl.GetReadCallCount() == 1);
	BOOST_TEST(test_serial_port_impl.GetTotalBytesRead() == test_data.size());
}

BOOST_FIXTURE_TEST_CASE(MockSerialPort_ReadError, ProtocolHandlerTestFixture)
{
	test_serial_port_impl.QueueReadError(boost::asio::error::eof);

	std::array<uint8_t, 16> buffer{};
	boost::system::error_code ec;
	[[maybe_unused]] auto bytes_read = serial_port.read_some(boost::asio::buffer(buffer.data(), buffer.size()), ec);

	BOOST_TEST(ec == boost::asio::error::eof);
	BOOST_TEST(test_serial_port_impl.GetReadErrorCount() == 1);
}

BOOST_FIXTURE_TEST_CASE(MockSerialPort_WriteSuccess, ProtocolHandlerTestFixture)
{
	std::vector<uint8_t> test_data = { 0x10, 0x02, 0x42, 0x03, 0x10, 0x03 };
	test_serial_port_impl.QueueWriteResponse(test_data.size());

	boost::system::error_code ec;
	auto bytes_written = serial_port.write_some(boost::asio::buffer(test_data.data(), test_data.size()), ec);

	BOOST_TEST(!ec);
	BOOST_TEST(bytes_written == test_data.size());
	BOOST_TEST(test_serial_port_impl.GetWriteCallCount() == 1);
	BOOST_TEST(test_serial_port_impl.GetWrittenData() == test_data);
}

BOOST_FIXTURE_TEST_CASE(MockSerialPort_WritePartial, ProtocolHandlerTestFixture)
{
	std::vector<uint8_t> test_data = { 0x10, 0x02, 0x42, 0x03, 0x10, 0x03 };
	test_serial_port_impl.QueueWriteResponse(3); // Only 3 bytes written

	boost::system::error_code ec;
	auto bytes_written = serial_port.write_some(boost::asio::buffer(test_data.data(), test_data.size()), ec);

	BOOST_TEST(!ec);
	BOOST_TEST(bytes_written == 3);
	BOOST_TEST(bytes_written < test_data.size());
}

BOOST_FIXTURE_TEST_CASE(MockSerialPort_WriteError, ProtocolHandlerTestFixture)
{
	std::vector<uint8_t> test_data = { 0x10, 0x02, 0x42, 0x03 };
	test_serial_port_impl.QueueWriteError(boost::asio::error::broken_pipe);

	boost::system::error_code ec;
	[[maybe_unused]] auto bytes_written = serial_port.write_some(boost::asio::buffer(test_data.data(), test_data.size()), ec);

	BOOST_TEST(ec == boost::asio::error::broken_pipe);
	BOOST_TEST(test_serial_port_impl.GetWriteErrorCount() == 1);
}

BOOST_FIXTURE_TEST_CASE(MockSerialPort_MultipleReads, ProtocolHandlerTestFixture)
{
	std::vector<uint8_t> data1 = { 0x10, 0x02, 0x42 };
	std::vector<uint8_t> data2 = { 0x03, 0x10, 0x03 };

	test_serial_port_impl.QueueReadData(data1);
	test_serial_port_impl.QueueReadData(data2);

	std::array<uint8_t, 16> buffer{};
	boost::system::error_code ec;

	// First read
	auto result1 = serial_port.read_some(boost::asio::buffer(buffer.data(), buffer.size()), ec);
	BOOST_TEST(!ec);
	BOOST_TEST(result1 == data1.size());

	// Second read
	auto result2 = serial_port.read_some(boost::asio::buffer(buffer.data(), buffer.size()), ec);
	BOOST_TEST(!ec);
	BOOST_TEST(result2 == data2.size());

	BOOST_TEST(test_serial_port_impl.GetReadCallCount() == 2);
	BOOST_TEST(test_serial_port_impl.GetTotalBytesRead() == data1.size() + data2.size());
}

BOOST_FIXTURE_TEST_CASE(MockSerialPort_ResetClears, ProtocolHandlerTestFixture)
{
	test_serial_port_impl.QueueReadData({ 0x01, 0x02 });
	test_serial_port_impl.QueueWriteResponse(10);

	test_serial_port_impl.Reset();

	BOOST_TEST(test_serial_port_impl.IsReadQueueEmpty() == true);
	BOOST_TEST(test_serial_port_impl.IsWriteQueueEmpty() == true);
	BOOST_TEST(test_serial_port_impl.GetReadCallCount() == 0);
	BOOST_TEST(test_serial_port_impl.GetWriteCallCount() == 0);
}

BOOST_AUTO_TEST_CASE(MessageBuilder_CreateValidMessage)
{
	auto message = Test::MessageBuilder::CreateValidMessage(0x42, 0x03, { 0x01, 0x02 });

	BOOST_TEST(message.size() == 8); // header(2) + dest(1) + cmd(1) + payload(2) + footer(2)
	BOOST_TEST(message[0] == 0x10);
	BOOST_TEST(message[1] == 0x02);
	BOOST_TEST(message[2] == 0x42); // destination
	BOOST_TEST(message[3] == 0x03); // command
	BOOST_TEST(message[4] == 0x01); // payload
	BOOST_TEST(message[5] == 0x02); // payload
	BOOST_TEST(message[6] == 0x10);
	BOOST_TEST(message[7] == 0x03);
}

BOOST_AUTO_TEST_CASE(MessageBuilder_CreatePartialMessage)
{
	auto message = Test::MessageBuilder::CreatePartialMessage(5);

	BOOST_TEST(message.size() == 5);
	BOOST_TEST(message[0] == 0x10);
	BOOST_TEST(message[1] == 0x02);
	BOOST_TEST(message[2] == 0x42);
}

BOOST_AUTO_TEST_CASE(MessageBuilder_CreateMultipleMessages)
{
	auto messages = Test::MessageBuilder::CreateMultipleMessages(3);

	// Each message has header(2) + dest(1) + cmd(1) + payload(1) + footer(2) = 7 bytes
	BOOST_TEST(messages.size() == 3 * 7);

	// Verify first message starts correctly
	BOOST_TEST(messages[0] == 0x10);
	BOOST_TEST(messages[1] == 0x02);
}

BOOST_AUTO_TEST_CASE(MessageBuilder_CreateGarbageData)
{
	auto garbage = Test::MessageBuilder::CreateGarbageData(100);

	BOOST_TEST(garbage.size() == 100);
	// Verify it's not all zeros or all the same value
	bool has_variation = false;
	for (std::size_t i = 1; i < garbage.size(); ++i)
	{
		if (garbage[i] != garbage[0])
		{
			has_variation = true;
			break;
		}
	}
	BOOST_TEST(has_variation == true);
}

BOOST_FIXTURE_TEST_CASE(Scenario_CompleteMessage_SingleRead, ProtocolHandlerTestFixture)
{
	// Queue a complete message in one read
	auto message = Test::MessageBuilder::CreateValidMessage(0x42, 0x03, { 0x01, 0x02, 0x03 });
	test_serial_port_impl.QueueReadData(message);

	std::array<uint8_t, 16> buffer{};
	boost::system::error_code ec;
	auto bytes_read = serial_port.read_some(boost::asio::buffer(buffer.data(), buffer.size()), ec);

	BOOST_TEST(!ec);
	BOOST_TEST(bytes_read == message.size());

	// Verify the data matches
	for (std::size_t i = 0; i < message.size(); ++i)
	{
		BOOST_TEST(buffer[i] == message[i]);
	}
}

BOOST_FIXTURE_TEST_CASE(Scenario_PartialMessage_RequiresMultipleReads, ProtocolHandlerTestFixture)
{
	// Queue a message split across multiple reads
	std::vector<uint8_t> part1 = { 0x10, 0x02, 0x42 };
	std::vector<uint8_t> part2 = { 0x03, 0x01, 0x10, 0x03 };

	test_serial_port_impl.QueueReadData(part1);
	test_serial_port_impl.QueueReadData(part2);

	std::array<uint8_t, 16> buffer1{}, buffer2{};
	boost::system::error_code ec;

	auto result1 = serial_port.read_some(boost::asio::buffer(buffer1.data(), buffer1.size()), ec);
	BOOST_TEST(!ec);
	BOOST_TEST(result1 == part1.size());

	auto result2 = serial_port.read_some(boost::asio::buffer(buffer2.data(), buffer2.size()), ec);
	BOOST_TEST(!ec);
	BOOST_TEST(result2 == part2.size());

	// Together they form a complete message
	BOOST_TEST(test_serial_port_impl.GetTotalBytesRead() == part1.size() + part2.size());
}

BOOST_FIXTURE_TEST_CASE(Scenario_MultipleMessages_SingleRead, ProtocolHandlerTestFixture)
{
	// Queue multiple messages in one read
	auto messages = Test::MessageBuilder::CreateMultipleMessages(3);
	test_serial_port_impl.QueueReadData(messages);

	std::array<uint8_t, 64> buffer{};
	boost::system::error_code ec;
	auto bytes_read = serial_port.read_some(boost::asio::buffer(buffer.data(), buffer.size()), ec);

	BOOST_TEST(!ec);
	BOOST_TEST(bytes_read == messages.size());
}

BOOST_FIXTURE_TEST_CASE(Scenario_GarbageData_BeforeValidMessage, ProtocolHandlerTestFixture)
{
	// Queue garbage followed by valid message
	auto garbage = Test::MessageBuilder::CreateGarbageData(10);
	auto message = Test::MessageBuilder::CreateValidMessage(0x42, 0x03, { 0x01 });

	std::vector<uint8_t> combined;
	combined.insert(combined.end(), garbage.begin(), garbage.end());
	combined.insert(combined.end(), message.begin(), message.end());

	test_serial_port_impl.QueueReadData(combined);

	std::array<uint8_t, 64> buffer{};
	boost::system::error_code ec;
	auto bytes_read = serial_port.read_some(boost::asio::buffer(buffer.data(), buffer.size()), ec);

	BOOST_TEST(!ec);
	BOOST_TEST(bytes_read == combined.size());
}

BOOST_FIXTURE_TEST_CASE(Scenario_ReadWriteCycle, ProtocolHandlerTestFixture)
{
	// Simulate reading a message, processing it, and writing a response
	auto incoming_message = Test::MessageBuilder::CreateValidMessage(0x42, 0x03, { 0x01 });
	auto response_message = Test::MessageBuilder::CreateValidMessage(0x00, 0x05, { 0x02 });

	test_serial_port_impl.QueueReadData(incoming_message);
	test_serial_port_impl.QueueWriteResponse(response_message.size());

	// Read
	std::array<uint8_t, 16> read_buffer{};
	boost::system::error_code ec;
	auto read_result = serial_port.read_some(boost::asio::buffer(read_buffer.data(), read_buffer.size()), ec);
	BOOST_TEST(!ec);
	BOOST_TEST(read_result > 0);

	// Write response
	auto write_result = serial_port.write_some(boost::asio::buffer(response_message.data(), response_message.size()), ec);
	BOOST_TEST(!ec);
	BOOST_TEST(write_result > 0);

	BOOST_TEST(test_serial_port_impl.GetReadCallCount() == 1);
	BOOST_TEST(test_serial_port_impl.GetWriteCallCount() == 1);
}

BOOST_FIXTURE_TEST_CASE(Error_ReadEOF_ShouldTerminate, ProtocolHandlerTestFixture)
{
	test_serial_port_impl.QueueReadError(boost::asio::error::eof);

	std::array<uint8_t, 16> buffer{};
	boost::system::error_code ec;
	serial_port.read_some(boost::asio::buffer(buffer.data(), buffer.size()), ec);

	BOOST_TEST(ec == boost::asio::error::eof);
}

BOOST_FIXTURE_TEST_CASE(Error_ReadBadDescriptor, ProtocolHandlerTestFixture)
{
	test_serial_port_impl.close();
	test_serial_port_impl.EnableTestMode(false); // Use actual mock behavior

	std::array<uint8_t, 16> buffer{};
	boost::system::error_code ec;
	serial_port.read_some(boost::asio::buffer(buffer.data(), buffer.size()), ec);

	BOOST_TEST(ec == boost::asio::error::bad_descriptor);
}

BOOST_FIXTURE_TEST_CASE(Statistics_TrackReadCalls, ProtocolHandlerTestFixture)
{
	test_serial_port_impl.QueueReadData({ 0x01, 0x02 });
	test_serial_port_impl.QueueReadData({ 0x03, 0x04 });
	test_serial_port_impl.QueueReadData({ 0x05, 0x06 });

	std::array<uint8_t, 16> buffer{};
	boost::system::error_code ec;

	serial_port.read_some(boost::asio::buffer(buffer.data(), buffer.size()), ec);
	serial_port.read_some(boost::asio::buffer(buffer.data(), buffer.size()), ec);
	serial_port.read_some(boost::asio::buffer(buffer.data(), buffer.size()), ec);

	BOOST_TEST(test_serial_port_impl.GetReadCallCount() == 3);
	BOOST_TEST(test_serial_port_impl.GetTotalBytesRead() == 6);
}

BOOST_FIXTURE_TEST_CASE(Statistics_TrackWriteCalls, ProtocolHandlerTestFixture)
{
	test_serial_port_impl.QueueWriteResponse(10);
	test_serial_port_impl.QueueWriteResponse(20);

	std::vector<uint8_t> data1(10, 0xAA);
	std::vector<uint8_t> data2(20, 0xBB);
	boost::system::error_code ec;

	serial_port.write_some(boost::asio::buffer(data1.data(), data1.size()), ec);
	serial_port.write_some(boost::asio::buffer(data2.data(), data2.size()), ec);

	BOOST_TEST(test_serial_port_impl.GetWriteCallCount() == 2);
	BOOST_TEST(test_serial_port_impl.GetTotalBytesWritten() == 30);
}

BOOST_FIXTURE_TEST_CASE(Statistics_ErrorCounting, ProtocolHandlerTestFixture)
{
	test_serial_port_impl.QueueReadError(boost::asio::error::eof);
	test_serial_port_impl.QueueReadData({ 0x01, 0x02 });
	test_serial_port_impl.QueueReadError(boost::asio::error::broken_pipe);

	std::array<uint8_t, 16> buffer{};
	boost::system::error_code ec;

	serial_port.read_some(boost::asio::buffer(buffer.data(), buffer.size()), ec);
	serial_port.read_some(boost::asio::buffer(buffer.data(), buffer.size()), ec);
	serial_port.read_some(boost::asio::buffer(buffer.data(), buffer.size()), ec);

	BOOST_TEST(test_serial_port_impl.GetReadErrorCount() == 2);
	BOOST_TEST(test_serial_port_impl.GetReadCallCount() == 3);
}

BOOST_AUTO_TEST_SUITE_END()
