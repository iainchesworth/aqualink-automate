#include <string>

#include <boost/test/unit_test.hpp>

#include "exceptions/exception_developer_optionkeyhasnovalue.h"
#include "exceptions/exception_developer_optionkeyinvalidtype.h"
#include "exceptions/exception_developer_optionkeynotprovided.h"
#include "exceptions/exception_http_duplicateroute.h"
#include "exceptions/exception_options_conflictingoptions.h"
#include "exceptions/exception_options_missingdependency.h"
#include "exceptions/exception_webserver.h"

// Regression coverage for exception constructors that previously discarded
// their `message` argument (forwarding a placeholder constant to the base).
// Only the constructors that remain in use are covered here; the unused
// single-arg constructors on other exception classes were removed outright.

BOOST_AUTO_TEST_SUITE(TestSuite_ExceptionMessageForwarding)

BOOST_AUTO_TEST_CASE(Test_Developer_OptionKeyHasNoValue_ForwardsMessage)
{
	const std::string msg{ "custom detail for Developer_OptionKeyHasNoValue" };
	AqualinkAutomate::Exceptions::Developer_OptionKeyHasNoValue ex(msg);

	BOOST_CHECK_EQUAL(msg, ex.What());
	BOOST_CHECK_EQUAL(msg, std::string(ex.what()));
}

BOOST_AUTO_TEST_CASE(Test_Developer_OptionKeyInvalidType_ForwardsMessage)
{
	const std::string msg{ "custom detail for Developer_OptionKeyInvalidType" };
	AqualinkAutomate::Exceptions::Developer_OptionKeyInvalidType ex(msg);

	BOOST_CHECK_EQUAL(msg, ex.What());
	BOOST_CHECK_EQUAL(msg, std::string(ex.what()));
}

BOOST_AUTO_TEST_CASE(Test_Developer_OptionKeyNotProvided_ForwardsMessage)
{
	const std::string msg{ "custom detail for Developer_OptionKeyNotProvided" };
	AqualinkAutomate::Exceptions::Developer_OptionKeyNotProvided ex(msg);

	BOOST_CHECK_EQUAL(msg, ex.What());
	BOOST_CHECK_EQUAL(msg, std::string(ex.what()));
}

BOOST_AUTO_TEST_CASE(Test_Options_ConflictingOptions_ForwardsMessage)
{
	const std::string msg{ "custom detail for Options_ConflictingOptions" };
	AqualinkAutomate::Exceptions::Options_ConflictingOptions ex(msg);

	BOOST_CHECK_EQUAL(msg, ex.What());
	BOOST_CHECK_EQUAL(msg, std::string(ex.what()));
}

BOOST_AUTO_TEST_CASE(Test_Options_MissingDependency_ForwardsMessage)
{
	const std::string msg{ "custom detail for Options_MissingDependency" };
	AqualinkAutomate::Exceptions::Options_MissingDependency ex(msg);

	BOOST_CHECK_EQUAL(msg, ex.What());
	BOOST_CHECK_EQUAL(msg, std::string(ex.what()));
}

BOOST_AUTO_TEST_CASE(Test_WebServerException_ForwardsMessage)
{
	const std::string msg{ "custom detail for WebServerException" };
	AqualinkAutomate::Exceptions::WebServerException ex(msg);

	BOOST_CHECK_EQUAL(msg, ex.What());
	BOOST_CHECK_EQUAL(msg, std::string(ex.what()));
}

BOOST_AUTO_TEST_CASE(Test_HTTP_DuplicateRoute_ForwardsMessage)
{
	const std::string msg{ "custom detail for HTTP_DuplicateRoute" };
	AqualinkAutomate::Exceptions::HTTP_DuplicateRoute ex(msg);

	BOOST_CHECK_EQUAL(msg, ex.What());
	BOOST_CHECK_EQUAL(msg, std::string(ex.what()));
}

BOOST_AUTO_TEST_SUITE_END()
