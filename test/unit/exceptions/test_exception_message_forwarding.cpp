#include <string>

#include <boost/test/unit_test.hpp>

#include "exceptions/exception_certificate_invalidformat.h"
#include "exceptions/exception_certificate_notfound.h"
#include "exceptions/exception_developer_optionkeyhasnovalue.h"
#include "exceptions/exception_developer_optionkeyinvalidtype.h"
#include "exceptions/exception_developer_optionkeynotprovided.h"
#include "exceptions/exception_http_duplicateroute.h"
#include "exceptions/exception_hubnotfound.h"
#include "exceptions/exception_notimplemented.h"
#include "exceptions/exception_options_conflictingoptions.h"
#include "exceptions/exception_options_missingdependency.h"
#include "exceptions/exception_signallingstatscounter_badaccess.h"
#include "exceptions/exception_traits_doesnotexist.h"
#include "exceptions/exception_traits_failedtoset.h"
#include "exceptions/exception_traits_invalidtraitvalue.h"
#include "exceptions/exception_traits_notmutable.h"
#include "exceptions/exception_webserver.h"

BOOST_AUTO_TEST_SUITE(TestSuite_ExceptionMessageForwarding)

BOOST_AUTO_TEST_CASE(Test_Certificate_InvalidFormat_ForwardsMessage)
{
	const std::string msg{ "custom detail for Certificate_InvalidFormat" };
	AqualinkAutomate::Exceptions::Certificate_InvalidFormat ex(msg);

	BOOST_CHECK_EQUAL(msg, ex.What());
	BOOST_CHECK_EQUAL(msg, std::string(ex.what()));
}

BOOST_AUTO_TEST_CASE(Test_Certificate_NotFound_ForwardsMessage)
{
	const std::string msg{ "custom detail for Certificate_NotFound" };
	AqualinkAutomate::Exceptions::Certificate_NotFound ex(msg);

	BOOST_CHECK_EQUAL(msg, ex.What());
	BOOST_CHECK_EQUAL(msg, std::string(ex.what()));
}

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

BOOST_AUTO_TEST_CASE(Test_Hub_NotFound_ForwardsMessage)
{
	const std::string msg{ "custom detail for Hub_NotFound" };
	AqualinkAutomate::Exceptions::Hub_NotFound ex(msg);

	BOOST_CHECK_EQUAL(msg, ex.What());
	BOOST_CHECK_EQUAL(msg, std::string(ex.what()));
}

BOOST_AUTO_TEST_CASE(Test_NotImplemented_ForwardsMessage)
{
	const std::string msg{ "custom detail for NotImplemented" };
	AqualinkAutomate::Exceptions::NotImplemented ex(msg);

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

BOOST_AUTO_TEST_CASE(Test_SignallingStatsCounter_BadAccess_ForwardsMessage)
{
	const std::string msg{ "custom detail for SignallingStatsCounter_BadAccess" };
	AqualinkAutomate::Exceptions::SignallingStatsCounter_BadAccess ex(msg);

	BOOST_CHECK_EQUAL(msg, ex.What());
	BOOST_CHECK_EQUAL(msg, std::string(ex.what()));
}

BOOST_AUTO_TEST_CASE(Test_Traits_DoesNotExist_ForwardsMessage)
{
	const std::string msg{ "custom detail for Traits_DoesNotExist" };
	AqualinkAutomate::Exceptions::Traits_DoesNotExist ex(msg);

	BOOST_CHECK_EQUAL(msg, ex.What());
	BOOST_CHECK_EQUAL(msg, std::string(ex.what()));
}

BOOST_AUTO_TEST_CASE(Test_Traits_FailedToSet_ForwardsMessage)
{
	const std::string msg{ "custom detail for Traits_FailedToSet" };
	AqualinkAutomate::Exceptions::Traits_FailedToSet ex(msg);

	BOOST_CHECK_EQUAL(msg, ex.What());
	BOOST_CHECK_EQUAL(msg, std::string(ex.what()));
}

BOOST_AUTO_TEST_CASE(Test_Traits_InvalidTraitValue_ForwardsMessage)
{
	const std::string msg{ "custom detail for Traits_InvalidTraitValue" };
	AqualinkAutomate::Exceptions::Traits_InvalidTraitValue ex(msg);

	BOOST_CHECK_EQUAL(msg, ex.What());
	BOOST_CHECK_EQUAL(msg, std::string(ex.what()));
}

BOOST_AUTO_TEST_CASE(Test_Traits_NotMutable_ForwardsMessage)
{
	const std::string msg{ "custom detail for Traits_NotMutable" };
	AqualinkAutomate::Exceptions::Traits_NotMutable ex(msg);

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
