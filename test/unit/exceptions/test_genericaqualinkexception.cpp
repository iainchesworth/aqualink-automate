#include <boost/test/unit_test.hpp>

#include "exceptions/exception_genericaqualinkexception.h"

BOOST_AUTO_TEST_SUITE(TestSuite_Exceptions)

BOOST_AUTO_TEST_CASE(Test_Exceptions_Properties)
{
	using AqualinkAutomate::Exceptions::GenericAqualinkException;

	const std::string exception_message{ "This is a test message" };

	auto raise_exception = [&exception_message]()
		{
			throw GenericAqualinkException(exception_message);
		};

	auto check_exception_message = [&exception_message](const auto& ex) -> auto
		{
			return (exception_message == ex.What());
		};

	auto check_exception_sourcelocation = [](const auto& ex) -> auto
		{
			auto& source_location = ex.Where();

			bool all_checks_successful = true;
			// Verify line number is captured (exact value depends on formatting/compiler)
			all_checks_successful &= (source_location.line() > 0);
			// Verify column is captured (exact value varies by compiler/formatting)
			all_checks_successful &= (source_location.column() >= 0);
			// Verify file name contains expected test file
			all_checks_successful &= (std::string::npos != std::string(source_location.file_name()).find("test_genericaqualinkexception.cpp"));

			return all_checks_successful;
		};

	BOOST_CHECK_EXCEPTION(raise_exception(), GenericAqualinkException, check_exception_message);
	BOOST_CHECK_EXCEPTION(raise_exception(), GenericAqualinkException, check_exception_sourcelocation);

}

BOOST_AUTO_TEST_SUITE_END()
