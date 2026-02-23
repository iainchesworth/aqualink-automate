#include <cstdint>
#include <vector>

#include <boost/asio/buffer.hpp>
#include <boost/asio/error.hpp>
#include <boost/system/error_code.hpp>
#include <boost/test/unit_test.hpp>

#include "mocks/mock_testserialportimpl.h"

using namespace AqualinkAutomate::Test;

BOOST_AUTO_TEST_SUITE(TestSerialPortImpl_TestSuite)

// =============================================================================
// Construction
// =============================================================================

BOOST_AUTO_TEST_CASE(TestConstruction_OpensAutomatically)
{
	TestSerialPortImpl impl;
	BOOST_CHECK(impl.is_open());
}

BOOST_AUTO_TEST_CASE(TestConstruction_QueuesEmpty)
{
	TestSerialPortImpl impl;
	BOOST_CHECK(impl.IsReadQueueEmpty());
	BOOST_CHECK(impl.IsWriteQueueEmpty());
}

BOOST_AUTO_TEST_CASE(TestConstruction_StatisticsZero)
{
	TestSerialPortImpl impl;
	BOOST_CHECK_EQUAL(impl.GetReadCallCount(), 0);
	BOOST_CHECK_EQUAL(impl.GetWriteCallCount(), 0);
	BOOST_CHECK_EQUAL(impl.GetTotalBytesRead(), 0);
	BOOST_CHECK_EQUAL(impl.GetTotalBytesWritten(), 0);
	BOOST_CHECK_EQUAL(impl.GetReadErrorCount(), 0);
	BOOST_CHECK_EQUAL(impl.GetWriteErrorCount(), 0);
}

// =============================================================================
// Test mode: queued reads
// =============================================================================

BOOST_AUTO_TEST_CASE(TestQueueRead_ThenRead_ReturnsQueuedData)
{
	TestSerialPortImpl impl;
	impl.EnableTestMode();

	std::vector<uint8_t> expected = {0x10, 0x02, 0x48, 0x02};
	impl.QueueReadData(expected);

	uint8_t buf[16]{};
	boost::system::error_code ec;
	auto bytes = impl.read_some(boost::asio::buffer(buf), ec);

	BOOST_CHECK(!ec);
	BOOST_CHECK_EQUAL(bytes, expected.size());
	BOOST_CHECK_EQUAL(buf[0], 0x10);
	BOOST_CHECK_EQUAL(buf[1], 0x02);
	BOOST_CHECK_EQUAL(buf[2], 0x48);
	BOOST_CHECK_EQUAL(buf[3], 0x02);
}

BOOST_AUTO_TEST_CASE(TestQueueRead_MultipleReads_FIFO)
{
	TestSerialPortImpl impl;
	impl.EnableTestMode();

	impl.QueueReadData({0xAA});
	impl.QueueReadData({0xBB});

	uint8_t buf[16]{};
	boost::system::error_code ec;

	impl.read_some(boost::asio::buffer(buf), ec);
	BOOST_CHECK_EQUAL(buf[0], 0xAA);

	impl.read_some(boost::asio::buffer(buf), ec);
	BOOST_CHECK_EQUAL(buf[0], 0xBB);
}

BOOST_AUTO_TEST_CASE(TestQueueReadError_ReturnsError)
{
	TestSerialPortImpl impl;
	impl.EnableTestMode();

	impl.QueueReadError(boost::asio::error::connection_reset);

	uint8_t buf[16]{};
	boost::system::error_code ec;
	auto bytes = impl.read_some(boost::asio::buffer(buf), ec);

	BOOST_CHECK_EQUAL(bytes, 0);
	BOOST_CHECK_EQUAL(ec, boost::asio::error::connection_reset);
	BOOST_CHECK_EQUAL(impl.GetReadErrorCount(), 1);
}

BOOST_AUTO_TEST_CASE(TestQueueReadBatch_ReturnsAll)
{
	TestSerialPortImpl impl;
	impl.EnableTestMode();

	std::vector<std::vector<uint8_t>> batch = {{0x01, 0x02}, {0x03, 0x04}, {0x05}};
	impl.QueueReadDataBatch(batch);

	uint8_t buf[16]{};
	boost::system::error_code ec;

	auto bytes1 = impl.read_some(boost::asio::buffer(buf), ec);
	BOOST_CHECK_EQUAL(bytes1, 2);
	BOOST_CHECK_EQUAL(buf[0], 0x01);

	auto bytes2 = impl.read_some(boost::asio::buffer(buf), ec);
	BOOST_CHECK_EQUAL(bytes2, 2);
	BOOST_CHECK_EQUAL(buf[0], 0x03);

	auto bytes3 = impl.read_some(boost::asio::buffer(buf), ec);
	BOOST_CHECK_EQUAL(bytes3, 1);
	BOOST_CHECK_EQUAL(buf[0], 0x05);
}

// =============================================================================
// Test mode: queued writes
// =============================================================================

BOOST_AUTO_TEST_CASE(TestQueueWrite_ReturnsQueuedByteCount)
{
	TestSerialPortImpl impl;
	impl.EnableTestMode();

	impl.QueueWriteResponse(3);

	uint8_t data[] = {0x10, 0x02, 0x48};
	boost::system::error_code ec;
	auto bytes = impl.write_some(boost::asio::buffer(data), ec);

	BOOST_CHECK(!ec);
	BOOST_CHECK_EQUAL(bytes, 3);
}

BOOST_AUTO_TEST_CASE(TestQueueWriteError_ReturnsError)
{
	TestSerialPortImpl impl;
	impl.EnableTestMode();

	impl.QueueWriteError(boost::asio::error::connection_refused);

	uint8_t data[] = {0x10};
	boost::system::error_code ec;
	auto bytes = impl.write_some(boost::asio::buffer(data), ec);

	BOOST_CHECK_EQUAL(bytes, 0);
	BOOST_CHECK_EQUAL(ec, boost::asio::error::connection_refused);
	BOOST_CHECK_EQUAL(impl.GetWriteErrorCount(), 1);
}

// =============================================================================
// Written data capture
// =============================================================================

BOOST_AUTO_TEST_CASE(TestWrittenData_CapturesAllWrites)
{
	TestSerialPortImpl impl;
	impl.EnableTestMode();

	impl.QueueWriteResponse(2);
	impl.QueueWriteResponse(3);

	uint8_t data1[] = {0xAA, 0xBB};
	uint8_t data2[] = {0xCC, 0xDD, 0xEE};
	boost::system::error_code ec;

	impl.write_some(boost::asio::buffer(data1), ec);
	impl.write_some(boost::asio::buffer(data2), ec);

	auto& written = impl.GetWrittenData();
	BOOST_REQUIRE_EQUAL(written.size(), 5);
	BOOST_CHECK_EQUAL(written[0], 0xAA);
	BOOST_CHECK_EQUAL(written[1], 0xBB);
	BOOST_CHECK_EQUAL(written[2], 0xCC);
	BOOST_CHECK_EQUAL(written[3], 0xDD);
	BOOST_CHECK_EQUAL(written[4], 0xEE);
}

BOOST_AUTO_TEST_CASE(TestClearWrittenData_ClearsCapture)
{
	TestSerialPortImpl impl;
	impl.EnableTestMode();
	impl.QueueWriteResponse(1);

	uint8_t data[] = {0xFF};
	boost::system::error_code ec;
	impl.write_some(boost::asio::buffer(data), ec);

	BOOST_REQUIRE(!impl.GetWrittenData().empty());
	impl.ClearWrittenData();
	BOOST_CHECK(impl.GetWrittenData().empty());
}

// =============================================================================
// Statistics tracking
// =============================================================================

BOOST_AUTO_TEST_CASE(TestStatistics_ReadCallCount)
{
	TestSerialPortImpl impl;
	impl.EnableTestMode();
	impl.QueueReadData({0x01});
	impl.QueueReadData({0x02});

	uint8_t buf[16]{};
	boost::system::error_code ec;
	impl.read_some(boost::asio::buffer(buf), ec);
	impl.read_some(boost::asio::buffer(buf), ec);

	BOOST_CHECK_EQUAL(impl.GetReadCallCount(), 2);
	BOOST_CHECK_EQUAL(impl.GetTotalBytesRead(), 2);
}

BOOST_AUTO_TEST_CASE(TestStatistics_WriteCallCount)
{
	TestSerialPortImpl impl;
	impl.EnableTestMode();
	impl.QueueWriteResponse(2);

	uint8_t data[] = {0x01, 0x02};
	boost::system::error_code ec;
	impl.write_some(boost::asio::buffer(data), ec);

	BOOST_CHECK_EQUAL(impl.GetWriteCallCount(), 1);
	BOOST_CHECK_EQUAL(impl.GetTotalBytesWritten(), 2);
}

BOOST_AUTO_TEST_CASE(TestResetStatistics_ClearsAll)
{
	TestSerialPortImpl impl;
	impl.EnableTestMode();
	impl.QueueReadData({0x01});
	impl.QueueWriteResponse(1);

	uint8_t buf[16]{};
	uint8_t data[] = {0x01};
	boost::system::error_code ec;
	impl.read_some(boost::asio::buffer(buf), ec);
	impl.write_some(boost::asio::buffer(data), ec);

	impl.ResetStatistics();
	BOOST_CHECK_EQUAL(impl.GetReadCallCount(), 0);
	BOOST_CHECK_EQUAL(impl.GetWriteCallCount(), 0);
	BOOST_CHECK_EQUAL(impl.GetTotalBytesRead(), 0);
	BOOST_CHECK_EQUAL(impl.GetTotalBytesWritten(), 0);
}

// =============================================================================
// Reset
// =============================================================================

BOOST_AUTO_TEST_CASE(TestReset_ClearsEverything)
{
	TestSerialPortImpl impl;
	impl.EnableTestMode();
	impl.QueueReadData({0x01});
	impl.QueueWriteResponse(1);

	uint8_t buf[16]{};
	uint8_t data[] = {0x01};
	boost::system::error_code ec;
	impl.read_some(boost::asio::buffer(buf), ec);
	impl.write_some(boost::asio::buffer(data), ec);

	impl.Reset();
	BOOST_CHECK(impl.IsReadQueueEmpty());
	BOOST_CHECK(impl.IsWriteQueueEmpty());
	BOOST_CHECK(impl.GetWrittenData().empty());
	BOOST_CHECK_EQUAL(impl.GetReadCallCount(), 0);
}

// =============================================================================
// Fallback to base class when test mode disabled
// =============================================================================

BOOST_AUTO_TEST_CASE(TestNoTestMode_Read_FallsBackToMockData)
{
	TestSerialPortImpl impl;
	// Test mode not enabled, queued data ignored

	impl.QueueReadData({0xAA, 0xBB});

	uint8_t buf[16]{};
	boost::system::error_code ec;
	auto bytes = impl.read_some(boost::asio::buffer(buf), ec);

	// Falls back to random mock data from base class
	BOOST_CHECK(!ec);
	BOOST_CHECK(bytes > 0);
	BOOST_CHECK_EQUAL(impl.GetReadCallCount(), 1);
}

BOOST_AUTO_TEST_SUITE_END()
