#include <cstdint>
#include <memory>

#include <boost/asio/io_context.hpp>
#include <boost/test/unit_test.hpp>

#include "protocol/protocol_handler.h"

#include "mocks/mock_generator_messagetorawdata.h"
#include "mocks/mock_generator_rawdatatomessage.h"
#include "mocks/mock_serialport.h"

using namespace AqualinkAutomate::Developer;
using namespace AqualinkAutomate::Messages;
using namespace AqualinkAutomate::Protocol;

BOOST_AUTO_TEST_SUITE(ProtocolHandlerSuite)

struct MockRawData {
    // Implement required functionality
};

class MockSerialPort : public Developer::mock_serial_port
{
    // Implement required functionality
};

BOOST_AUTO_TEST_CASE(HandlePublishTest)
{
   /* boost::asio::io_context ioc;
    Test::MockSerialPort serial_port;
    Test::MockGenerator_RawDataToMessage generator_message;
    Test::MockGenerator_MessageToRawData generator_rawdata;

    Protocol::ProtocolHandler handler(ioc, serial_port, generator_message, generator_rawdata);

    MockMessage message;

    BOOST_TEST(handler.HandlePublish(message) == true);*/
}

BOOST_AUTO_TEST_CASE(ConstructorDestructorTest)
{
   /* boost::asio::io_context ioc;
    Test::MockSerialPort serial_port;
    MockMessage generator_message;
    MockRawData generator_rawdata;

    Protocol::ProtocolHandler<MockMessage, MockRawData>* handler = new AqualinkAutomate::Protocol::ProtocolHandler<MockMessage, MockRawData>(ioc, serial_port, generator_message, generator_rawdata);
    delete handler;
    BOOST_TEST(true); */
}

BOOST_AUTO_TEST_SUITE_END()
