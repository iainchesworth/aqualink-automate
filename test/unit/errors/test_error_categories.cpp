#include <string>

#include <boost/system/error_code.hpp>
#include <boost/test/unit_test.hpp>

#include "errors/enum_error_category.h"
#include "errors/message_errors.h"
#include "errors/protocol_errors.h"

using namespace AqualinkAutomate::ErrorCodes;

BOOST_AUTO_TEST_SUITE(TestSuite_ErrorCategories)

// =====================================================
// Message Error Category
//
// message() now returns a human-readable description (previously it returned
// the C++ enumerator spelling).  These expectations were updated as part of
// the CRTP EnumErrorCategory refactor; the change is observable in logs/API.
// =====================================================

BOOST_AUTO_TEST_CASE(Test_MessageErrorCategory_Name)
{
	const auto& cat = Message_ErrorCategory::Instance();
	BOOST_CHECK_EQUAL(std::string(cat.name()), "AqualinkAutomate::Message Error Category");
}

BOOST_AUTO_TEST_CASE(Test_MessageErrorCategory_InvalidMessageData)
{
	const auto& cat = Message_ErrorCategory::Instance();
	BOOST_CHECK_EQUAL(cat.message(static_cast<int>(Message_ErrorCodes::Error_InvalidMessageData)), "The serial data did not form a valid message and could not be decoded");
}

BOOST_AUTO_TEST_CASE(Test_MessageErrorCategory_CannotFindGenerator)
{
	const auto& cat = Message_ErrorCategory::Instance();
	BOOST_CHECK_EQUAL(cat.message(static_cast<int>(Message_ErrorCodes::Error_CannotFindGenerator)), "No registered message generator recognised the serial data");
}

BOOST_AUTO_TEST_CASE(Test_MessageErrorCategory_UnknownMessageType)
{
	const auto& cat = Message_ErrorCategory::Instance();
	BOOST_CHECK_EQUAL(cat.message(static_cast<int>(Message_ErrorCodes::Error_UnknownMessageType)), "The message type is not recognised by the factory");
}

BOOST_AUTO_TEST_CASE(Test_MessageErrorCategory_GeneratorFailed)
{
	const auto& cat = Message_ErrorCategory::Instance();
	BOOST_CHECK_EQUAL(cat.message(static_cast<int>(Message_ErrorCodes::Error_GeneratorFailed)), "The message generator failed to produce a message from the serial data");
}

BOOST_AUTO_TEST_CASE(Test_MessageErrorCategory_FailedToSerialize)
{
	const auto& cat = Message_ErrorCategory::Instance();
	BOOST_CHECK_EQUAL(cat.message(static_cast<int>(Message_ErrorCodes::Error_FailedToSerialize)), "The message could not be serialised to wire bytes");
}

BOOST_AUTO_TEST_CASE(Test_MessageErrorCategory_FailedToDeserialize)
{
	const auto& cat = Message_ErrorCategory::Instance();
	BOOST_CHECK_EQUAL(cat.message(static_cast<int>(Message_ErrorCodes::Error_FailedToDeserialize)), "The serial data could not be deserialised into the message");
}

BOOST_AUTO_TEST_CASE(Test_MessageErrorCategory_UnknownCode)
{
	const auto& cat = Message_ErrorCategory::Instance();
	BOOST_CHECK_EQUAL(cat.message(9999), "AqualinkAutomate::Message Error Category - unknown error code (9999)");
}

BOOST_AUTO_TEST_CASE(Test_MessageErrorCategory_Singleton)
{
	const auto& cat1 = Message_ErrorCategory::Instance();
	const auto& cat2 = Message_ErrorCategory::Instance();
	BOOST_CHECK_EQUAL(&cat1, &cat2);
}

BOOST_AUTO_TEST_CASE(Test_MessageErrorCategory_MakeErrorCode)
{
	auto ec = make_error_code(Message_ErrorCodes::Error_InvalidMessageData);
	BOOST_CHECK_EQUAL(ec.value(), static_cast<int>(Message_ErrorCodes::Error_InvalidMessageData));
	BOOST_CHECK_EQUAL(&ec.category(), &Message_ErrorCategory::Instance());
}

BOOST_AUTO_TEST_CASE(Test_MessageErrorCategory_MakeErrorCondition)
{
	auto cond = make_error_condition(Message_ErrorCodes::Error_GeneratorFailed);
	BOOST_CHECK_EQUAL(cond.value(), static_cast<int>(Message_ErrorCodes::Error_GeneratorFailed));
	BOOST_CHECK_EQUAL(&cond.category(), &Message_ErrorCategory::Instance());
}

// =====================================================
// Protocol Error Category
// =====================================================

BOOST_AUTO_TEST_CASE(Test_ProtocolErrorCategory_Name)
{
	const auto& cat = Protocol_ErrorCategory::Instance();
	BOOST_CHECK_EQUAL(std::string(cat.name()), "AqualinkAutomate::Protocol Error Category");
}

BOOST_AUTO_TEST_CASE(Test_ProtocolErrorCategory_DataAvailableToProcess)
{
	const auto& cat = Protocol_ErrorCategory::Instance();
	BOOST_CHECK_EQUAL(cat.message(static_cast<int>(Protocol_ErrorCodes::DataAvailableToProcess)), "A complete frame is available in the buffer and ready to process");
}

BOOST_AUTO_TEST_CASE(Test_ProtocolErrorCategory_WaitingForMoreData)
{
	const auto& cat = Protocol_ErrorCategory::Instance();
	BOOST_CHECK_EQUAL(cat.message(static_cast<int>(Protocol_ErrorCodes::WaitingForMoreData)), "Not enough data has been received yet to decode a frame");
}

BOOST_AUTO_TEST_CASE(Test_ProtocolErrorCategory_InvalidPacketFormat)
{
	const auto& cat = Protocol_ErrorCategory::Instance();
	BOOST_CHECK_EQUAL(cat.message(static_cast<int>(Protocol_ErrorCodes::InvalidPacketFormat)), "The packet structure does not match the expected protocol format");
}

BOOST_AUTO_TEST_CASE(Test_ProtocolErrorCategory_UnknownFailure)
{
	const auto& cat = Protocol_ErrorCategory::Instance();
	BOOST_CHECK_EQUAL(cat.message(static_cast<int>(Protocol_ErrorCodes::UnknownFailure)), "An unspecified protocol processing failure occurred");
}

BOOST_AUTO_TEST_CASE(Test_ProtocolErrorCategory_ChecksumFailure)
{
	const auto& cat = Protocol_ErrorCategory::Instance();
	BOOST_CHECK_EQUAL(cat.message(static_cast<int>(Protocol_ErrorCodes::ChecksumFailure)), "The packet checksum did not match the computed value");
}

BOOST_AUTO_TEST_CASE(Test_ProtocolErrorCategory_OverlappingPackets)
{
	const auto& cat = Protocol_ErrorCategory::Instance();
	BOOST_CHECK_EQUAL(cat.message(static_cast<int>(Protocol_ErrorCodes::OverlappingPackets)), "Overlapping packet boundaries were detected in the buffer");
}

BOOST_AUTO_TEST_CASE(Test_ProtocolErrorCategory_UnknownCode)
{
	const auto& cat = Protocol_ErrorCategory::Instance();
	BOOST_CHECK_EQUAL(cat.message(9999), "AqualinkAutomate::Protocol Error Category - unknown error code (9999)");
}

BOOST_AUTO_TEST_CASE(Test_ProtocolErrorCategory_Singleton)
{
	const auto& cat1 = Protocol_ErrorCategory::Instance();
	const auto& cat2 = Protocol_ErrorCategory::Instance();
	BOOST_CHECK_EQUAL(&cat1, &cat2);
}

BOOST_AUTO_TEST_CASE(Test_ProtocolErrorCategory_MakeErrorCode)
{
	auto ec = make_error_code(Protocol_ErrorCodes::InvalidPacketFormat);
	BOOST_CHECK_EQUAL(ec.value(), static_cast<int>(Protocol_ErrorCodes::InvalidPacketFormat));
	BOOST_CHECK_EQUAL(&ec.category(), &Protocol_ErrorCategory::Instance());
}

BOOST_AUTO_TEST_CASE(Test_ProtocolErrorCategory_MakeErrorCondition)
{
	auto cond = make_error_condition(Protocol_ErrorCodes::ChecksumFailure);
	BOOST_CHECK_EQUAL(cond.value(), static_cast<int>(Protocol_ErrorCodes::ChecksumFailure));
	BOOST_CHECK_EQUAL(&cond.category(), &Protocol_ErrorCategory::Instance());
}

// =====================================================
// Categories are distinct
// =====================================================

BOOST_AUTO_TEST_CASE(Test_CategoriesAreDistinct)
{
	const auto& msg_cat = Message_ErrorCategory::Instance();
	const auto& proto_cat = Protocol_ErrorCategory::Instance();
	BOOST_CHECK_NE(&msg_cat, static_cast<const boost::system::error_category*>(&proto_cat));
}

// =====================================================
// CRTP base: error_code values, categories and cross-category comparison
//
// Regression coverage for the consume-or-defer registry contract that relies
// on whole-error_code (value AND category) equality.  A 2001 from one category
// must NOT compare equal to a 2001 from another.
// =====================================================

BOOST_AUTO_TEST_CASE(Test_MakeErrorCode_PreservesEnumValue)
{
	BOOST_CHECK_EQUAL(make_error_code(Protocol_ErrorCodes::WaitingForMoreData).value(), 2001);
	BOOST_CHECK_EQUAL(make_error_code(Message_ErrorCodes::Error_CannotFindGenerator).value(), 1001);
}

BOOST_AUTO_TEST_CASE(Test_SameCode_ComparesEqual)
{
	const auto a = make_error_code(Protocol_ErrorCodes::WaitingForMoreData);
	const auto b = make_error_code(Protocol_ErrorCodes::WaitingForMoreData);
	BOOST_CHECK(a == b);
}

BOOST_AUTO_TEST_CASE(Test_DifferentCodeSameCategory_ComparesUnequal)
{
	const auto a = make_error_code(Protocol_ErrorCodes::WaitingForMoreData);
	const auto b = make_error_code(Protocol_ErrorCodes::ChecksumFailure);
	BOOST_CHECK(a != b);
}

BOOST_AUTO_TEST_SUITE_END()
