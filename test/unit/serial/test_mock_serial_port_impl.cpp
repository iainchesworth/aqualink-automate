#include <cstdint>
#include <vector>

#include <boost/asio/buffer.hpp>
#include <boost/asio/error.hpp>
#include <boost/system/error_code.hpp>
#include <boost/test/unit_test.hpp>

#include "developer/mock_serial_port_impl.h"

using namespace AqualinkAutomate::Developer;

BOOST_AUTO_TEST_SUITE(MockSerialPortImpl_TestSuite)

// =============================================================================
// Construction
// =============================================================================

BOOST_AUTO_TEST_CASE(TestDefaultConstruction_NotOpen)
{
	MockSerialPortImpl impl;
	BOOST_CHECK(!impl.is_open());
}

BOOST_AUTO_TEST_CASE(TestConstruction_WithDeviceName_Opens)
{
	MockSerialPortImpl impl("test_device");
	BOOST_CHECK(impl.is_open());
}

BOOST_AUTO_TEST_CASE(TestConstruction_WithDeviceNameAndEc_Opens)
{
	boost::system::error_code ec;
	MockSerialPortImpl impl("test_device", ec);
	BOOST_CHECK(impl.is_open());
	BOOST_CHECK(!ec);
}

// =============================================================================
// Open
// =============================================================================

BOOST_AUTO_TEST_CASE(TestOpen_SetsIsOpen)
{
	MockSerialPortImpl impl;
	boost::system::error_code ec;
	impl.open("test_device", ec);
	BOOST_CHECK(impl.is_open());
	BOOST_CHECK(!ec);
}

BOOST_AUTO_TEST_CASE(TestOpen_AlreadyOpen_ReturnsError)
{
	MockSerialPortImpl impl;
	boost::system::error_code ec;
	impl.open("test_device", ec);
	BOOST_REQUIRE(!ec);

	impl.open("test_device_2", ec);
	BOOST_CHECK_EQUAL(ec, boost::asio::error::already_open);
}

BOOST_AUTO_TEST_CASE(TestOpen_Throwing_WhenClosed)
{
	MockSerialPortImpl impl;
	BOOST_CHECK_NO_THROW(impl.open("test_device"));
	BOOST_CHECK(impl.is_open());
}

// =============================================================================
// Close
// =============================================================================

BOOST_AUTO_TEST_CASE(TestClose_SetsNotOpen)
{
	MockSerialPortImpl impl;
	boost::system::error_code ec;
	impl.open("test_device", ec);
	BOOST_REQUIRE(impl.is_open());

	impl.close(ec);
	BOOST_CHECK(!impl.is_open());
	BOOST_CHECK(!ec);
}

BOOST_AUTO_TEST_CASE(TestClose_WhenNotOpen_NoError)
{
	MockSerialPortImpl impl;
	boost::system::error_code ec;
	impl.close(ec);
	BOOST_CHECK(!ec);
}

BOOST_AUTO_TEST_CASE(TestClose_Throwing_DoesNotThrowWhenOpen)
{
	MockSerialPortImpl impl("test_device");
	BOOST_CHECK_NO_THROW(impl.close());
}

// =============================================================================
// Cancel
// =============================================================================

BOOST_AUTO_TEST_CASE(TestCancel_WhenOpen_NoError)
{
	MockSerialPortImpl impl("test_device");
	boost::system::error_code ec;
	impl.cancel(ec);
	BOOST_CHECK(!ec);
}

BOOST_AUTO_TEST_CASE(TestCancel_WhenOpen_ClearsPreExistingError)
{
	// Regression: cancel must clear ec on success even if caller passed a pre-set error
	MockSerialPortImpl impl("test_device");
	boost::system::error_code ec = boost::asio::error::operation_aborted;
	impl.cancel(ec);
	BOOST_CHECK(!ec);
}

BOOST_AUTO_TEST_CASE(TestCancel_WhenNotOpen_ReturnsBadDescriptor)
{
	MockSerialPortImpl impl;
	boost::system::error_code ec;
	impl.cancel(ec);
	BOOST_CHECK_EQUAL(ec, boost::asio::error::bad_descriptor);
}

BOOST_AUTO_TEST_CASE(TestCancel_Throwing_WhenNotOpen_Throws)
{
	MockSerialPortImpl impl;
	BOOST_CHECK_THROW(impl.cancel(), boost::system::system_error);
}

// =============================================================================
// Read operations
// =============================================================================

BOOST_AUTO_TEST_CASE(TestReadSome_WhenNotOpen_ReturnsBadDescriptor)
{
	MockSerialPortImpl impl;
	uint8_t buf[16];
	boost::system::error_code ec;
	auto bytes = impl.read_some(boost::asio::buffer(buf), ec);
	BOOST_CHECK_EQUAL(bytes, 0);
	BOOST_CHECK_EQUAL(ec, boost::asio::error::bad_descriptor);
}

BOOST_AUTO_TEST_CASE(TestReadSome_MockData_ReturnsSomeBytes)
{
	MockSerialPortImpl impl("test_device");
	uint8_t buf[64]{};
	boost::system::error_code ec;
	auto bytes = impl.read_some(boost::asio::buffer(buf), ec);
	BOOST_CHECK(!ec);
	BOOST_CHECK(bytes > 0);
	BOOST_CHECK(bytes <= 16); // mock reads up to 16 bytes
}

BOOST_AUTO_TEST_CASE(TestReadSome_Throwing_WhenNotOpen_Throws)
{
	MockSerialPortImpl impl;
	uint8_t buf[16];
	BOOST_CHECK_THROW(impl.read_some(boost::asio::buffer(buf)), boost::system::system_error);
}

BOOST_AUTO_TEST_CASE(TestReadSome_Throwing_WhenOpen_DoesNotThrow)
{
	MockSerialPortImpl impl("test_device");
	uint8_t buf[16]{};
	BOOST_CHECK_NO_THROW(impl.read_some(boost::asio::buffer(buf)));
}

// =============================================================================
// Write operations
// =============================================================================

BOOST_AUTO_TEST_CASE(TestWriteSome_WhenNotOpen_ReturnsBadDescriptor)
{
	MockSerialPortImpl impl;
	uint8_t data[] = {0x10, 0x02, 0x00};
	boost::system::error_code ec;
	auto bytes = impl.write_some(boost::asio::buffer(data), ec);
	BOOST_CHECK_EQUAL(bytes, 0);
	BOOST_CHECK_EQUAL(ec, boost::asio::error::bad_descriptor);
}

BOOST_AUTO_TEST_CASE(TestWriteSome_WhenOpen_ReturnsBufferSize)
{
	MockSerialPortImpl impl("test_device");
	uint8_t data[] = {0x10, 0x02, 0x00};
	boost::system::error_code ec;

	// Set baud rate to make the mock write fast
	impl.set_baud_rate(115200, ec);
	BOOST_REQUIRE(!ec);

	auto bytes = impl.write_some(boost::asio::buffer(data), ec);
	BOOST_CHECK(!ec);
	BOOST_CHECK_EQUAL(bytes, sizeof(data));
}

BOOST_AUTO_TEST_CASE(TestWriteSome_Throwing_WhenNotOpen_Throws)
{
	MockSerialPortImpl impl;
	uint8_t data[] = {0x10, 0x02};
	BOOST_CHECK_THROW(impl.write_some(boost::asio::buffer(data)), boost::system::system_error);
}

// =============================================================================
// Serial port options
// =============================================================================

BOOST_AUTO_TEST_CASE(TestSetBaudRate_DoesNotThrow)
{
	MockSerialPortImpl impl("test_device");
	boost::system::error_code ec;
	impl.set_baud_rate(9600, ec);
	BOOST_CHECK(!ec);
}

BOOST_AUTO_TEST_CASE(TestSetBaudRate_ClearsPreExistingError)
{
	// Regression: setters must clear ec on success
	MockSerialPortImpl impl("test_device");
	boost::system::error_code ec = boost::asio::error::operation_aborted;
	impl.set_baud_rate(9600, ec);
	BOOST_CHECK(!ec);
}

BOOST_AUTO_TEST_CASE(TestSetCharacterSize_DoesNotThrow)
{
	MockSerialPortImpl impl("test_device");
	boost::system::error_code ec;
	impl.set_character_size(8, ec);
	BOOST_CHECK(!ec);
}

BOOST_AUTO_TEST_CASE(TestSetCharacterSize_ClearsPreExistingError)
{
	MockSerialPortImpl impl("test_device");
	boost::system::error_code ec = boost::asio::error::operation_aborted;
	impl.set_character_size(8, ec);
	BOOST_CHECK(!ec);
}

BOOST_AUTO_TEST_CASE(TestSetReadTimeout_DoesNotThrow)
{
	MockSerialPortImpl impl("test_device");
	boost::system::error_code ec;
	impl.set_read_timeout(std::chrono::milliseconds(100), ec);
	BOOST_CHECK(!ec);
}

// =============================================================================
// Destruction
// =============================================================================

BOOST_AUTO_TEST_CASE(TestDestruction_WhenOpen_DoesNotThrow)
{
	{
		MockSerialPortImpl impl("test_device");
	}
	BOOST_CHECK(true);
}

BOOST_AUTO_TEST_CASE(TestDestruction_WhenClosed_DoesNotThrow)
{
	{
		MockSerialPortImpl impl;
	}
	BOOST_CHECK(true);
}

// =============================================================================
// Reopen after close
// =============================================================================

BOOST_AUTO_TEST_CASE(TestReopen_AfterClose_Succeeds)
{
	MockSerialPortImpl impl("test_device");
	boost::system::error_code ec;

	impl.close(ec);
	BOOST_REQUIRE(!ec);
	BOOST_CHECK(!impl.is_open());

	impl.open("test_device_2", ec);
	BOOST_CHECK(!ec);
	BOOST_CHECK(impl.is_open());
}

BOOST_AUTO_TEST_SUITE_END()
