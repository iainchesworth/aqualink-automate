#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "jandy/utility/jandy_checksum.h"

BOOST_AUTO_TEST_SUITE(JandyChecksum)

BOOST_AUTO_TEST_CASE(EmptyMessage_VectorUint8)
{
    using AqualinkAutomate::Utility::JandyPacket_CalculateChecksum;
    using AqualinkAutomate::Utility::JandyPacket_CalculateChecksum_FromRange;

    std::vector<uint8_t> message;

    // Range-based overload
    BOOST_CHECK_EQUAL(JandyPacket_CalculateChecksum_FromRange(message), 0x00);

    // Iterator-pair overload
    BOOST_CHECK_EQUAL(JandyPacket_CalculateChecksum(message.begin(), message.end()), 0x00);
}

BOOST_AUTO_TEST_CASE(SingleByteMessage_VectorUint8)
{
    using AqualinkAutomate::Utility::JandyPacket_CalculateChecksum;
    using AqualinkAutomate::Utility::JandyPacket_CalculateChecksum_FromRange;

    std::vector<uint8_t> message = { 0x7F };

    // Range-based overload
    BOOST_CHECK_EQUAL(JandyPacket_CalculateChecksum_FromRange(message), 0x7F);

    // Iterator-pair overload
    BOOST_CHECK_EQUAL(JandyPacket_CalculateChecksum(message.begin(), message.end()), 0x7F);
}

BOOST_AUTO_TEST_CASE(MultipleByteMessage_VectorUint8)
{
    using AqualinkAutomate::Utility::JandyPacket_CalculateChecksum;
    using AqualinkAutomate::Utility::JandyPacket_CalculateChecksum_FromRange;

    std::vector<uint8_t> message = { 0x01, 0x02, 0x03, 0x04 };

    // 0x01 + 0x02 + 0x03 + 0x04 = 0x0A
    BOOST_CHECK_EQUAL(JandyPacket_CalculateChecksum_FromRange(message), 0x0A);
    BOOST_CHECK_EQUAL(JandyPacket_CalculateChecksum(message.begin(), message.end()), 0x0A);
}

BOOST_AUTO_TEST_CASE(ChecksumOverflow_VectorUint8)
{
    using AqualinkAutomate::Utility::JandyPacket_CalculateChecksum;
    using AqualinkAutomate::Utility::JandyPacket_CalculateChecksum_FromRange;

    std::vector<uint8_t> message = { 0xFF, 0xFF, 0xFF, 0xFF };

    // 0xFF * 4 = 0x3FC -> low 8 bits = 0xFC
    BOOST_CHECK_EQUAL(JandyPacket_CalculateChecksum_FromRange(message), 0xFC);
    BOOST_CHECK_EQUAL(JandyPacket_CalculateChecksum(message.begin(), message.end()), 0xFC);
}

BOOST_AUTO_TEST_CASE(DifferentContainerTypes_Uint8)
{
    using AqualinkAutomate::Utility::JandyPacket_CalculateChecksum;
    using AqualinkAutomate::Utility::JandyPacket_CalculateChecksum_FromRange;

    std::array<uint8_t, 4> message_array = { 0x01, 0x02, 0x03, 0x04 };

    // Range-based overload on std::array
    BOOST_CHECK_EQUAL(JandyPacket_CalculateChecksum_FromRange(message_array), 0x0A);

    // Iterator-pair overload on std::array
    BOOST_CHECK_EQUAL(JandyPacket_CalculateChecksum(message_array.begin(), message_array.end()), 0x0A);

    // Also test a subrange (non-owning view)
    auto subrange = std::ranges::subrange(message_array.begin(), message_array.end());
    BOOST_CHECK_EQUAL(JandyPacket_CalculateChecksum_FromRange(subrange), 0x0A);
}

BOOST_AUTO_TEST_CASE(IntegralWideningAndTruncation)
{
    using AqualinkAutomate::Utility::JandyPacket_CalculateChecksum_FromRange;

    // Larger integral type - should be truncated to uint8_t internally
    std::vector<uint16_t> message_u16 = { 0x0001u, 0x0002u, 0x0003u, 0x0004u };
    BOOST_CHECK_EQUAL(JandyPacket_CalculateChecksum_FromRange(message_u16), 0x0A);

    // Signed type - casting to uint8_t should preserve low 8 bits
    std::vector<int> message_int = { 0x01, 0x02, 0x03, 0x04 };
    BOOST_CHECK_EQUAL(JandyPacket_CalculateChecksum_FromRange(message_int), 0x0A);

    // Signed values that overflow into negative domain, still low 8 bits used
    std::vector<int> message_int_overflow = { 0xFF, 0xFF, 0xFF, 0xFF };
    BOOST_CHECK_EQUAL(JandyPacket_CalculateChecksum_FromRange(message_int_overflow), 0xFC);
}

BOOST_AUTO_TEST_CASE(StdByteContainers)
{
    using AqualinkAutomate::Utility::JandyPacket_CalculateChecksum;
    using AqualinkAutomate::Utility::JandyPacket_CalculateChecksum_FromRange;

    // std::vector<std::byte>
    std::vector<std::byte> message_vec =
    {
        std::byte{ 0x01 },
        std::byte{ 0x02 },
        std::byte{ 0x03 },
        std::byte{ 0x04 }
    };

    BOOST_CHECK_EQUAL(JandyPacket_CalculateChecksum_FromRange(message_vec), 0x0A);
    BOOST_CHECK_EQUAL(JandyPacket_CalculateChecksum(message_vec.begin(), message_vec.end()), 0x0A);

    // std::array<std::byte, N>
    std::array<std::byte, 4> message_array =
    {
        std::byte{ 0xFF },
        std::byte{ 0xFF },
        std::byte{ 0xFF },
        std::byte{ 0xFF }
    };

    BOOST_CHECK_EQUAL(JandyPacket_CalculateChecksum_FromRange(message_array), 0xFC);
    BOOST_CHECK_EQUAL(JandyPacket_CalculateChecksum(message_array.begin(), message_array.end()), 0xFC);
}

BOOST_AUTO_TEST_CASE(IteratorSubrange_EmptyAndPartial)
{
    using AqualinkAutomate::Utility::JandyPacket_CalculateChecksum_FromRange;

    std::vector<uint8_t> message = { 0x10, 0x20, 0x30, 0x40 };

    // Empty subrange
    auto empty_subrange = std::ranges::subrange(message.begin(), message.begin());
    BOOST_CHECK_EQUAL(JandyPacket_CalculateChecksum_FromRange(empty_subrange), 0x00);

    // Partial subrange [1,3) -> 0x20 + 0x30 = 0x50
    auto partial_subrange = std::ranges::subrange(std::next(message.begin()), std::next(message.begin(), 3));
    BOOST_CHECK_EQUAL(JandyPacket_CalculateChecksum_FromRange(partial_subrange), 0x50);
}

BOOST_AUTO_TEST_SUITE_END();
