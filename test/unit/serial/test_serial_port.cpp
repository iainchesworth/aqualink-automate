#include <cstdint>
#include <vector>

#include <boost/asio/buffer.hpp>
#include <boost/system/error_code.hpp>
#include <boost/test/unit_test.hpp>

#include "mocks/mock_testserialportimpl.h"

#include "support/unit_test_ostream_support.h"

using namespace AqualinkAutomate::Test;

BOOST_AUTO_TEST_SUITE(SerialPort_TestSuite)

// =============================================================================
// Construction
// =============================================================================

BOOST_AUTO_TEST_CASE(TestConstruction_OpensTestDevice)
{
	TestSerialPortImpl impl;
	// The constructor opens "test_device" - should be operational
	BOOST_CHECK_EQUAL(impl.GetReadCallCount(), static_cast<size_t>(0));
	BOOST_CHECK_EQUAL(impl.GetWriteCallCount(), static_cast<size_t>(0));
}

// =============================================================================
// Read: Queue-based delegation
// =============================================================================

BOOST_AUTO_TEST_CASE(TestRead_QueuedData_Delegation)
{
	TestSerialPortImpl impl;
	impl.EnableTestMode(true);

	std::vector<uint8_t> expected_data = { 0x10, 0x02, 0x33, 0x00 };
	impl.QueueReadData(expected_data);

	std::vector<uint8_t> buffer(64);
	boost::system::error_code ec;
	auto bytes_read = impl.read_some(boost::asio::buffer(buffer), ec);

	BOOST_CHECK(!ec);
	BOOST_CHECK_EQUAL(bytes_read, expected_data.size());
	BOOST_CHECK_EQUAL(impl.GetReadCallCount(), static_cast<size_t>(1));
	BOOST_CHECK_EQUAL(impl.GetTotalBytesRead(), expected_data.size());

	// Verify data content
	for (size_t i = 0; i < expected_data.size(); ++i)
	{
		BOOST_CHECK_EQUAL(buffer[i], expected_data[i]);
	}
}

BOOST_AUTO_TEST_CASE(TestRead_MultipleQueuedReads)
{
	TestSerialPortImpl impl;
	impl.EnableTestMode(true);

	std::vector<uint8_t> data1 = { 0xAA, 0xBB };
	std::vector<uint8_t> data2 = { 0xCC, 0xDD, 0xEE };
	impl.QueueReadData(data1);
	impl.QueueReadData(data2);

	std::vector<uint8_t> buffer(64);
	boost::system::error_code ec;

	auto bytes1 = impl.read_some(boost::asio::buffer(buffer), ec);
	BOOST_CHECK(!ec);
	BOOST_CHECK_EQUAL(bytes1, data1.size());

	auto bytes2 = impl.read_some(boost::asio::buffer(buffer), ec);
	BOOST_CHECK(!ec);
	BOOST_CHECK_EQUAL(bytes2, data2.size());

	BOOST_CHECK_EQUAL(impl.GetReadCallCount(), static_cast<size_t>(2));
	BOOST_CHECK_EQUAL(impl.GetTotalBytesRead(), data1.size() + data2.size());
}

// =============================================================================
// Write: Queue-based delegation
// =============================================================================

BOOST_AUTO_TEST_CASE(TestWrite_QueuedResponse_Delegation)
{
	TestSerialPortImpl impl;
	impl.EnableTestMode(true);

	std::vector<uint8_t> write_data = { 0x10, 0x02, 0x50 };
	impl.QueueWriteResponse(write_data.size());

	boost::system::error_code ec;
	auto bytes_written = impl.write_some(boost::asio::buffer(write_data), ec);

	BOOST_CHECK(!ec);
	BOOST_CHECK_EQUAL(bytes_written, write_data.size());
	BOOST_CHECK_EQUAL(impl.GetWriteCallCount(), static_cast<size_t>(1));
	BOOST_CHECK_EQUAL(impl.GetTotalBytesWritten(), write_data.size());
}

BOOST_AUTO_TEST_CASE(TestWrite_CapturesWrittenData)
{
	TestSerialPortImpl impl;
	impl.EnableTestMode(true);

	std::vector<uint8_t> write_data = { 0xDE, 0xAD, 0xBE, 0xEF };
	impl.QueueWriteResponse(write_data.size());

	boost::system::error_code ec;
	impl.write_some(boost::asio::buffer(write_data), ec);

	const auto& written = impl.GetWrittenData();
	BOOST_REQUIRE_EQUAL(written.size(), write_data.size());
	for (size_t i = 0; i < write_data.size(); ++i)
	{
		BOOST_CHECK_EQUAL(written[i], write_data[i]);
	}
}

// =============================================================================
// Error Forwarding
// =============================================================================

BOOST_AUTO_TEST_CASE(TestRead_ErrorForwarding)
{
	TestSerialPortImpl impl;
	impl.EnableTestMode(true);

	auto ec_to_queue = boost::asio::error::make_error_code(boost::asio::error::eof);
	impl.QueueReadError(ec_to_queue);

	std::vector<uint8_t> buffer(64);
	boost::system::error_code ec;
	auto bytes_read = impl.read_some(boost::asio::buffer(buffer), ec);

	BOOST_CHECK(ec);
	BOOST_CHECK_EQUAL(bytes_read, static_cast<size_t>(0));
	BOOST_CHECK_EQUAL(impl.GetReadErrorCount(), static_cast<size_t>(1));
}

BOOST_AUTO_TEST_CASE(TestWrite_ErrorForwarding)
{
	TestSerialPortImpl impl;
	impl.EnableTestMode(true);

	auto ec_to_queue = boost::asio::error::make_error_code(boost::asio::error::broken_pipe);
	impl.QueueWriteError(ec_to_queue);

	std::vector<uint8_t> write_data = { 0x01, 0x02 };
	boost::system::error_code ec;
	auto bytes_written = impl.write_some(boost::asio::buffer(write_data), ec);

	BOOST_CHECK(ec);
	BOOST_CHECK_EQUAL(bytes_written, static_cast<size_t>(0));
	BOOST_CHECK_EQUAL(impl.GetWriteErrorCount(), static_cast<size_t>(1));
}

// =============================================================================
// Statistics & Reset
// =============================================================================

BOOST_AUTO_TEST_CASE(TestStatistics_IncrementCorrectly)
{
	TestSerialPortImpl impl;
	impl.EnableTestMode(true);

	// Queue and perform reads
	impl.QueueReadData({ 0x01 });
	impl.QueueReadData({ 0x02, 0x03 });

	std::vector<uint8_t> buffer(64);
	boost::system::error_code ec;
	impl.read_some(boost::asio::buffer(buffer), ec);
	impl.read_some(boost::asio::buffer(buffer), ec);

	BOOST_CHECK_EQUAL(impl.GetReadCallCount(), static_cast<size_t>(2));
	BOOST_CHECK_EQUAL(impl.GetTotalBytesRead(), static_cast<size_t>(3));

	// Queue and perform writes
	impl.QueueWriteResponse(2);
	std::vector<uint8_t> write_data = { 0xAA, 0xBB };
	impl.write_some(boost::asio::buffer(write_data), ec);

	BOOST_CHECK_EQUAL(impl.GetWriteCallCount(), static_cast<size_t>(1));
	BOOST_CHECK_EQUAL(impl.GetTotalBytesWritten(), static_cast<size_t>(2));
}

BOOST_AUTO_TEST_CASE(TestReset_ClearsEverything)
{
	TestSerialPortImpl impl;
	impl.EnableTestMode(true);

	impl.QueueReadData({ 0x01 });
	impl.QueueWriteResponse(1);

	std::vector<uint8_t> buffer(64);
	boost::system::error_code ec;
	impl.read_some(boost::asio::buffer(buffer), ec);

	std::vector<uint8_t> write_data = { 0xAA };
	impl.write_some(boost::asio::buffer(write_data), ec);

	impl.Reset();

	BOOST_CHECK_EQUAL(impl.GetReadCallCount(), static_cast<size_t>(0));
	BOOST_CHECK_EQUAL(impl.GetWriteCallCount(), static_cast<size_t>(0));
	BOOST_CHECK_EQUAL(impl.GetTotalBytesRead(), static_cast<size_t>(0));
	BOOST_CHECK_EQUAL(impl.GetTotalBytesWritten(), static_cast<size_t>(0));
	BOOST_CHECK(impl.IsReadQueueEmpty());
	BOOST_CHECK(impl.IsWriteQueueEmpty());
	BOOST_CHECK(impl.GetWrittenData().empty());
}

// =============================================================================
// Batch Reads
// =============================================================================

BOOST_AUTO_TEST_CASE(TestReadBatch_QueueMultipleAtOnce)
{
	TestSerialPortImpl impl;
	impl.EnableTestMode(true);

	std::vector<std::vector<uint8_t>> batch = {
		{ 0x01 },
		{ 0x02, 0x03 },
		{ 0x04, 0x05, 0x06 }
	};
	impl.QueueReadDataBatch(batch);

	std::vector<uint8_t> buffer(64);
	boost::system::error_code ec;

	auto bytes1 = impl.read_some(boost::asio::buffer(buffer), ec);
	BOOST_CHECK_EQUAL(bytes1, static_cast<size_t>(1));

	auto bytes2 = impl.read_some(boost::asio::buffer(buffer), ec);
	BOOST_CHECK_EQUAL(bytes2, static_cast<size_t>(2));

	auto bytes3 = impl.read_some(boost::asio::buffer(buffer), ec);
	BOOST_CHECK_EQUAL(bytes3, static_cast<size_t>(3));

	BOOST_CHECK_EQUAL(impl.GetTotalBytesRead(), static_cast<size_t>(6));
}

BOOST_AUTO_TEST_SUITE_END()
