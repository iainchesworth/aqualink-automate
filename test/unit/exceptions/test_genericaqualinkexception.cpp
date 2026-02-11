#include <boost/test/unit_test.hpp>

#include "exceptions/exception_genericaqualinkexception.h"

BOOST_AUTO_TEST_SUITE(TestSuite_Exceptions)

BOOST_AUTO_TEST_CASE(Test_Exceptions_MessageProperty)
{
	using AqualinkAutomate::Exceptions::GenericAqualinkException;

	const std::string exception_message{ "This is a test message" };
	GenericAqualinkException ex(exception_message);

	BOOST_CHECK_EQUAL(exception_message, ex.What());
}

BOOST_AUTO_TEST_CASE(Test_Exceptions_SourceLocationProperty)
{
	using AqualinkAutomate::Exceptions::GenericAqualinkException;

	GenericAqualinkException ex("test");
	auto& source_location = ex.Where();

	BOOST_CHECK_GT(source_location.line(), 0u);
	BOOST_CHECK_GE(source_location.column(), 0u);
	BOOST_CHECK_NE(std::string::npos, std::string(source_location.file_name()).find("test_genericaqualinkexception.cpp"));
}

BOOST_AUTO_TEST_CASE(Test_Exceptions_ThrowAndCatch)
{
	using AqualinkAutomate::Exceptions::GenericAqualinkException;

	const std::string exception_message{ "This is a test message" };

	BOOST_CHECK_THROW(throw GenericAqualinkException(exception_message), GenericAqualinkException);
}

BOOST_AUTO_TEST_SUITE_END()
