#include <array>
#include <cstdint>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "jandy/utility/jandy_checksum.h"

BOOST_AUTO_TEST_SUITE(JandyChecksum);

BOOST_AUTO_TEST_CASE(EmptyMessage)
{
    using AqualinkAutomate::Utility::JandyPacket_CalculateChecksum;

    std::vector<uint8_t> message;
    BOOST_CHECK_EQUAL(JandyPacket_CalculateChecksum(message), 0);
}

BOOST_AUTO_TEST_CASE(SingleByteMessage)
{
    using AqualinkAutomate::Utility::JandyPacket_CalculateChecksum;

    std::vector<uint8_t> message = { 0x7F };
    BOOST_CHECK_EQUAL(JandyPacket_CalculateChecksum(message), 0x7F);
}

BOOST_AUTO_TEST_CASE(MultipleByteMessage)
{
    using AqualinkAutomate::Utility::JandyPacket_CalculateChecksum;

    std::vector<uint8_t> message = { 0x01, 0x02, 0x03, 0x04 };
    BOOST_CHECK_EQUAL(JandyPacket_CalculateChecksum(message), 0x0A); // 0x01 + 0x02 + 0x03 + 0x04 = 0x0A
}

BOOST_AUTO_TEST_CASE(ChecksumOverflow)
{
    using AqualinkAutomate::Utility::JandyPacket_CalculateChecksum;

    std::vector<uint8_t> message = { 0xFF, 0xFF, 0xFF, 0xFF };
    BOOST_CHECK_EQUAL(JandyPacket_CalculateChecksum(message), 0xFC); // 0xFF + 0xFF + 0xFF + 0xFF = 0x3FC, but only lower 8 bits are returned
}

BOOST_AUTO_TEST_CASE(DifferentContainerTypes)
{
    using AqualinkAutomate::Utility::JandyPacket_CalculateChecksum;

    std::array<uint8_t, 4> message = { 0x01, 0x02, 0x03, 0x04 };
    BOOST_CHECK_EQUAL(JandyPacket_CalculateChecksum(message), 0x0A); // 0x01 + 0x02 + 0x03 + 0x04 = 0x0A
}

BOOST_AUTO_TEST_SUITE_END();
