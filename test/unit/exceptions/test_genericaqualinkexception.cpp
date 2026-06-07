#include <exception>
#include <stdexcept>

#include <boost/test/unit_test.hpp>

#include "exceptions/exception_developer_optionkeyhasnovalue.h"
#include "exceptions/exception_developer_optionkeyinvalidtype.h"
#include "exceptions/exception_genericaqualinkexception.h"
#include "exceptions/exception_hubnotfound.h"

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

BOOST_AUTO_TEST_CASE(Test_Exceptions_InheritsFromStdException)
{
	using AqualinkAutomate::Exceptions::GenericAqualinkException;

	GenericAqualinkException ex("test inheritance");

	// Verify dynamic_cast to std::exception succeeds
	std::exception* base_ptr = dynamic_cast<std::exception*>(&ex);
	BOOST_REQUIRE(base_ptr != nullptr);

	// Verify what() returns the expected string
	BOOST_CHECK_EQUAL(std::string(base_ptr->what()), "test inheritance");
}

BOOST_AUTO_TEST_CASE(Test_Exceptions_WhatMethodReturnsCorrectString)
{
	using AqualinkAutomate::Exceptions::GenericAqualinkException;

	const std::string msg{ "detailed error message" };
	GenericAqualinkException ex(msg);

	// what() (lowercase, std::exception override) should match What() content
	BOOST_CHECK_EQUAL(std::string(ex.what()), msg);
	BOOST_CHECK_EQUAL(ex.What(), msg);
}

BOOST_AUTO_TEST_CASE(Test_Exceptions_CatchableViaStdException)
{
	using AqualinkAutomate::Exceptions::GenericAqualinkException;

	bool caught_as_std_exception = false;

	try
	{
		throw GenericAqualinkException("catch me");
	}
	catch (const std::exception& e)
	{
		caught_as_std_exception = true;
		BOOST_CHECK_EQUAL(std::string(e.what()), "catch me");
	}

	BOOST_CHECK(caught_as_std_exception);
}

BOOST_AUTO_TEST_CASE(Test_Exceptions_DerivedTypeCatchableViaStdException)
{
	bool caught_as_std_exception = false;

	try
	{
		throw AqualinkAutomate::Exceptions::Hub_NotFound();
	}
	catch (const std::exception& e)
	{
		caught_as_std_exception = true;
		// Should have a non-empty what() message
		BOOST_CHECK(std::string(e.what()).size() > 0);
	}

	BOOST_CHECK(caught_as_std_exception);
}

BOOST_AUTO_TEST_CASE(Test_Exceptions_DeveloperOptionKeyHasNoValue_PreservesMessage)
{
	using AqualinkAutomate::Exceptions::Developer_OptionKeyHasNoValue;

	// Regression: the (const std::string&) constructor previously dropped its argument and
	// forwarded the hardcoded DEVELOPER_OPTIONS_KEY_HAS_NO_VALUE_MESSAGE placeholder to the
	// base class, so the formatted detail built at the call site (options_option_type.h) was lost.
	const std::string exception_message{ "Option pool-temp is present but has no value (empty)." };
	Developer_OptionKeyHasNoValue ex(exception_message);

	BOOST_CHECK_EQUAL(exception_message, ex.What());
	BOOST_CHECK_EQUAL(exception_message, std::string(ex.what()));
}

BOOST_AUTO_TEST_CASE(Test_Exceptions_DeveloperOptionKeyInvalidType_PreservesMessage)
{
	using AqualinkAutomate::Exceptions::Developer_OptionKeyInvalidType;

	// Regression: the (const std::string&) constructor previously dropped its argument and
	// forwarded the hardcoded DEVELOPER_OPTIONS_KEY_INVALID_TYPE_MESSAGE placeholder to the
	// base class, so the formatted detail built at the call site (options_option_type.h) was lost.
	const std::string exception_message{ "Type mismatch for option pool-temp; requested type: int" };
	Developer_OptionKeyInvalidType ex(exception_message);

	BOOST_CHECK_EQUAL(exception_message, ex.What());
	BOOST_CHECK_EQUAL(exception_message, std::string(ex.what()));
}

BOOST_AUTO_TEST_SUITE_END()
