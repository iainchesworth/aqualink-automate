#include <format>
#include <sstream>
#include <string>

#include <boost/test/unit_test.hpp>

#include <magic_enum/magic_enum.hpp>

#include "jandy/formatters/jandy_message_formatters.h"
#include "jandy/messages/jandy_message_ids.h"
#include "jandy/messages/jandy_message_ack.h"
#include "jandy/messages/jandy_message_message.h"
#include "jandy/messages/jandy_message_message_long.h"
#include "jandy/messages/jandy_message_probe.h"
#include "jandy/messages/jandy_message_status.h"
#include "jandy/messages/jandy_message_unknown.h"

#include "support/unit_test_ostream_support.h"

using namespace AqualinkAutomate::Messages;

BOOST_AUTO_TEST_SUITE(Formatters_JandyMessagesTestSuite)

// =============================================================================
// std::formatter<JandyMessageIds>
// =============================================================================

BOOST_AUTO_TEST_CASE(TestFormat_JandyMessageIds_KnownValue)
{
	auto result = std::format("{}", JandyMessageIds::Probe);
	auto expected = magic_enum::enum_name(JandyMessageIds::Probe);
	BOOST_CHECK_EQUAL(result, std::string(expected));
}

BOOST_AUTO_TEST_CASE(TestFormat_JandyMessageIds_Ack)
{
	auto result = std::format("{}", JandyMessageIds::Ack);
	BOOST_CHECK(!result.empty());
}

// =============================================================================
// std::formatter<JandyMessage>
// =============================================================================

BOOST_AUTO_TEST_CASE(TestFormat_JandyMessage_CallsToString)
{
	JandyMessage_Probe message;
	auto formatted = std::format("{}", static_cast<const JandyMessage&>(message));
	auto to_string = message.ToString();
	BOOST_CHECK_EQUAL(formatted, to_string);
}

// =============================================================================
// ostream operators
// =============================================================================

BOOST_AUTO_TEST_CASE(TestOstream_JandyMessageIds)
{
	std::ostringstream oss;
	oss << JandyMessageIds::Probe;
	BOOST_CHECK(!oss.str().empty());
	BOOST_CHECK_EQUAL(oss.str(), std::format("{}", JandyMessageIds::Probe));
}

BOOST_AUTO_TEST_CASE(TestOstream_JandyMessage_Ack)
{
	JandyMessage_Ack message;
	std::ostringstream oss;
	oss << message;
	BOOST_CHECK(!oss.str().empty());
}

BOOST_AUTO_TEST_CASE(TestOstream_JandyMessage_Message)
{
	JandyMessage_Message message;
	std::ostringstream oss;
	oss << message;
	BOOST_CHECK(!oss.str().empty());
}

BOOST_AUTO_TEST_CASE(TestOstream_JandyMessage_MessageLong)
{
	JandyMessage_MessageLong message;
	std::ostringstream oss;
	oss << message;
	BOOST_CHECK(!oss.str().empty());
}

BOOST_AUTO_TEST_CASE(TestOstream_JandyMessage_Probe)
{
	JandyMessage_Probe message;
	std::ostringstream oss;
	oss << message;
	BOOST_CHECK(!oss.str().empty());
}

BOOST_AUTO_TEST_CASE(TestOstream_JandyMessage_Status)
{
	JandyMessage_Status message;
	std::ostringstream oss;
	oss << message;
	BOOST_CHECK(!oss.str().empty());
}

BOOST_AUTO_TEST_CASE(TestOstream_JandyMessage_Unknown)
{
	JandyMessage_Unknown message;
	std::ostringstream oss;
	oss << message;
	BOOST_CHECK(!oss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()
