#include <boost/test/unit_test.hpp>

#include "jandy/utility/screen_data_page.h"
#include "jandy/utility/screen_data_page_processor.h"

#include "support/unit_test_ostream_support.h"

using namespace AqualinkAutomate::Utility;

BOOST_AUTO_TEST_SUITE(ScreenDataPageProcessor_TestSuite)

BOOST_AUTO_TEST_CASE(TestPageType)
{
    ScreenDataPage_Processor processor(ScreenDataPageTypes::Page_Home, { 0, "Home" }, [](const ScreenDataPage&) {});
    BOOST_CHECK_EQUAL(processor.PageType(), ScreenDataPageTypes::Page_Home);
}

BOOST_AUTO_TEST_CASE(TestCanProcess)
{
    ScreenDataPage_Processor processor(ScreenDataPageTypes::Page_Home, { 0, "Home" }, [](const ScreenDataPage&) {});
    ScreenDataPage page(5);
    
    page[0].Text = "The Home Page";
    BOOST_CHECK_EQUAL(processor.CanProcess(page), true);

    page[0].Text = "Service Page";
    BOOST_CHECK_EQUAL(processor.CanProcess(page), false);
}

BOOST_AUTO_TEST_CASE(TestProcess)
{
    bool processed = false;

    ScreenDataPage_Processor processor(ScreenDataPageTypes::Page_Home, { 0, "Home" }, [&processed](const ScreenDataPage&) { processed = true; });
    ScreenDataPage page(5);
    
    page[0].Text = "The Home Page";
    processor.Process(page);

    BOOST_CHECK_EQUAL(processed, true);

    processed = false;

    page[0].Text = "Service Page";
    processor.Process(page);

    BOOST_CHECK_EQUAL(processed, true); // The process is ALWAYS run irrespective of whether the page was the "right" page.
}

BOOST_AUTO_TEST_CASE(TestPageProcessorOutOfRangeLineId)
{
    ScreenDataPage_Processor processor(ScreenDataPageTypes::Page_Home, { 10, "Home" }, [](const ScreenDataPage&) {});

    ScreenDataPage page(5);
    page[0].Text = "Home";

    BOOST_CHECK_NO_THROW(processor.CanProcess(page));
}

BOOST_AUTO_TEST_SUITE_END()
