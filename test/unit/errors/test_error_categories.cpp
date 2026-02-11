#include <string>

#include <boost/system/error_code.hpp>
#include <boost/test/unit_test.hpp>

#include "errors/message_errors.h"
#include "errors/protocol_errors.h"

using namespace AqualinkAutomate::ErrorCodes;

BOOST_AUTO_TEST_SUITE(TestSuite_ErrorCategories)

// =====================================================
// Message Error Category
// =====================================================

BOOST_AUTO_TEST_CASE(Test_MessageErrorCategory_Name)
{
	const auto& cat = Message_ErrorCategory::Instance();
	BOOST_CHECK_EQUAL(std::string(cat.name()), "AqualinkAutomate::Message Error Category");
}

BOOST_AUTO_TEST_CASE(Test_MessageErrorCategory_InvalidMessageData)
{
	const auto& cat = Message_ErrorCategory::Instance();
	BOOST_CHECK_EQUAL(cat.message(Message_ErrorCodes::Error_InvalidMessageData), "Message_ErrorCodes::Error_InvalidMessageData");
}

BOOST_AUTO_TEST_CASE(Test_MessageErrorCategory_CannotFindGenerator)
{
	const auto& cat = Message_ErrorCategory::Instance();
	BOOST_CHECK_EQUAL(cat.message(Message_ErrorCodes::Error_CannotFindGenerator), "Message_ErrorCodes::Error_CannotFindGenerator");
}

BOOST_AUTO_TEST_CASE(Test_MessageErrorCategory_UnknownMessageType)
{
	const auto& cat = Message_ErrorCategory::Instance();
	BOOST_CHECK_EQUAL(cat.message(Message_ErrorCodes::Error_UnknownMessageType), "Message_ErrorCodes::Error_UnknownMessageType");
}

BOOST_AUTO_TEST_CASE(Test_MessageErrorCategory_GeneratorFailed)
{
	const auto& cat = Message_ErrorCategory::Instance();
	BOOST_CHECK_EQUAL(cat.message(Message_ErrorCodes::Error_GeneratorFailed), "Message_ErrorCodes::Error_GeneratorFailed");
}

BOOST_AUTO_TEST_CASE(Test_MessageErrorCategory_FailedToSerialize)
{
	const auto& cat = Message_ErrorCategory::Instance();
	BOOST_CHECK_EQUAL(cat.message(Message_ErrorCodes::Error_FailedToSerialize), "Message_ErrorCodes::Error_FailedToSerialize");
}

BOOST_AUTO_TEST_CASE(Test_MessageErrorCategory_FailedToDeserialize)
{
	const auto& cat = Message_ErrorCategory::Instance();
	BOOST_CHECK_EQUAL(cat.message(Message_ErrorCodes::Error_FailedToDeserialize), "Message_ErrorCodes::Error_FailedToDeserialize");
}

BOOST_AUTO_TEST_CASE(Test_MessageErrorCategory_UnknownCode)
{
	const auto& cat = Message_ErrorCategory::Instance();
	BOOST_CHECK_EQUAL(cat.message(9999), "Message_ErrorCodes - Unknown Error Code");
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
	BOOST_CHECK_EQUAL(cat.message(Protocol_ErrorCodes::DataAvailableToProcess), "Protocol_ErrorCodes::DataAvailableToProcess");
}

BOOST_AUTO_TEST_CASE(Test_ProtocolErrorCategory_WaitingForMoreData)
{
	const auto& cat = Protocol_ErrorCategory::Instance();
	BOOST_CHECK_EQUAL(cat.message(Protocol_ErrorCodes::WaitingForMoreData), "Protocol_ErrorCodes::WaitingForMoreData");
}

BOOST_AUTO_TEST_CASE(Test_ProtocolErrorCategory_InvalidPacketFormat)
{
	const auto& cat = Protocol_ErrorCategory::Instance();
	BOOST_CHECK_EQUAL(cat.message(Protocol_ErrorCodes::InvalidPacketFormat), "Protocol_ErrorCodes::InvalidPacketFormat");
}

BOOST_AUTO_TEST_CASE(Test_ProtocolErrorCategory_UnknownFailure)
{
	const auto& cat = Protocol_ErrorCategory::Instance();
	BOOST_CHECK_EQUAL(cat.message(Protocol_ErrorCodes::UnknownFailure), "Protocol_ErrorCodes::UnknownFailure");
}

BOOST_AUTO_TEST_CASE(Test_ProtocolErrorCategory_ChecksumFailure)
{
	const auto& cat = Protocol_ErrorCategory::Instance();
	BOOST_CHECK_EQUAL(cat.message(Protocol_ErrorCodes::ChecksumFailure), "Protocol_ErrorCodes::ChecksumFailure");
}

BOOST_AUTO_TEST_CASE(Test_ProtocolErrorCategory_OverlappingPackets)
{
	const auto& cat = Protocol_ErrorCategory::Instance();
	BOOST_CHECK_EQUAL(cat.message(Protocol_ErrorCodes::OverlappingPackets), "Protocol_ErrorCodes::OverlappingPackets");
}

BOOST_AUTO_TEST_CASE(Test_ProtocolErrorCategory_UnknownCode)
{
	const auto& cat = Protocol_ErrorCategory::Instance();
	BOOST_CHECK_EQUAL(cat.message(9999), "Protocol_ErrorCodes - Unknown Error Code");
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

BOOST_AUTO_TEST_SUITE_END()
