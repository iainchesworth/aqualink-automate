#include <boost/test/unit_test.hpp>

#include "jandy/auxillaries/jandy_auxillary_traits_types.h"
#include "jandy/factories/jandy_auxillary_factory.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"

#include "support/unit_test_ostream_support.h"

using namespace AqualinkAutomate;

BOOST_AUTO_TEST_SUITE(JandyAuxillaryFactory_TestSuite)

BOOST_AUTO_TEST_CASE(Instance_Test) 
{
    Factory::JandyAuxillaryFactory& instance1 = Factory::JandyAuxillaryFactory::Instance();
    Factory::JandyAuxillaryFactory& instance2 = Factory::JandyAuxillaryFactory::Instance();
    BOOST_TEST(&instance1 == &instance2);
}

BOOST_AUTO_TEST_CASE(SerialAdapterDevice_CreateDevice_Test) 
{
    Factory::JandyAuxillaryFactory& factory = Factory::JandyAuxillaryFactory::Instance();

    // Test with various Auxillary IDs and Statuses
    // Note: You should replace these with the valid IDs and statuses in your code

    /*auto aux_id = Auxillaries::SOME_VALID_ID;
    auto status = Auxillaries::SOME_VALID_STATUS;

    auto result1 = factory.SerialAdapterDevice_CreateDevice(aux_id);
    auto result2 = factory.SerialAdapterDevice_CreateDevice(aux_id, status);*/

    // Validate the results according to your expectations
    // BOOST_TEST(result1.has_value());
    // BOOST_TEST(result2.has_value());
}

BOOST_AUTO_TEST_CASE(OneTouchDevice_CreateDevice_Test) 
{
    Factory::JandyAuxillaryFactory& factory = Factory::JandyAuxillaryFactory::Instance();

    static const std::vector<std::string> valid_aux_label_and_status =
    {
        "Aux1         OFF",
        "Aux B1       OFF",
        "Extra Aux    OFF",
        "AquaPure     OFF",
        "Cleaner      OFF",
        "Heat Pump    OFF",
        "Pool Pump    OFF",
        "Spillover    OFF"
    };

    {
        auto test_ptr = factory.OneTouchDevice_CreateDevice(Utility::AuxillaryState(valid_aux_label_and_status[0]));
        BOOST_TEST_REQUIRE(test_ptr.has_value());
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::AuxillaryTypeTrait{}));
        BOOST_TEST_REQUIRE(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::AuxillaryTypeTrait{})) == AuxillaryTraitsTypes::AuxillaryTypes::Auxillary);
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(Auxillaries::JandyAuxillaryId{}));
        BOOST_TEST_REQUIRE(*(test_ptr.value()->AuxillaryTraits.Get(Auxillaries::JandyAuxillaryId{})) == Auxillaries::JandyAuxillaryIds::Aux_1);
    }

    {
        auto test_ptr = factory.OneTouchDevice_CreateDevice(Utility::AuxillaryState(valid_aux_label_and_status[1]));
        BOOST_TEST_REQUIRE(test_ptr.has_value());
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::AuxillaryTypeTrait{}));
        BOOST_TEST_REQUIRE(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::AuxillaryTypeTrait{})) == AuxillaryTraitsTypes::AuxillaryTypes::Auxillary);
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(Auxillaries::JandyAuxillaryId{}));
        BOOST_TEST_REQUIRE(*(test_ptr.value()->AuxillaryTraits.Get(Auxillaries::JandyAuxillaryId{})) == Auxillaries::JandyAuxillaryIds::Aux_B1);
    }

    {
        auto test_ptr = factory.OneTouchDevice_CreateDevice(Utility::AuxillaryState(valid_aux_label_and_status[2]));
        BOOST_TEST_REQUIRE(test_ptr.has_value());
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::AuxillaryTypeTrait{}));
        BOOST_TEST_REQUIRE(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::AuxillaryTypeTrait{})) == AuxillaryTraitsTypes::AuxillaryTypes::Auxillary);
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(Auxillaries::JandyAuxillaryId{}));
        BOOST_TEST_REQUIRE(*(test_ptr.value()->AuxillaryTraits.Get(Auxillaries::JandyAuxillaryId{})) == Auxillaries::JandyAuxillaryIds::ExtraAux);
    }
    
    {
        auto test_ptr = factory.OneTouchDevice_CreateDevice(Utility::AuxillaryState(valid_aux_label_and_status[3]));
        BOOST_TEST_REQUIRE(test_ptr.has_value());
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::AuxillaryTypeTrait{}));
        BOOST_TEST_REQUIRE(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::AuxillaryTypeTrait{})) == AuxillaryTraitsTypes::AuxillaryTypes::Chlorinator);
    }

    {
        auto test_ptr = factory.OneTouchDevice_CreateDevice(Utility::AuxillaryState(valid_aux_label_and_status[4]));
        BOOST_TEST_REQUIRE(test_ptr.has_value());
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::AuxillaryTypeTrait{}));
        BOOST_TEST_REQUIRE(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::AuxillaryTypeTrait{})) == AuxillaryTraitsTypes::AuxillaryTypes::Cleaner);
    }

    {
        auto test_ptr = factory.OneTouchDevice_CreateDevice(Utility::AuxillaryState(valid_aux_label_and_status[5]));
        BOOST_TEST_REQUIRE(test_ptr.has_value());
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::AuxillaryTypeTrait{}));
        BOOST_TEST_REQUIRE(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::AuxillaryTypeTrait{})) == AuxillaryTraitsTypes::AuxillaryTypes::Heater);
    }

    {
        auto test_ptr = factory.OneTouchDevice_CreateDevice(Utility::AuxillaryState(valid_aux_label_and_status[6]));
        BOOST_TEST_REQUIRE(test_ptr.has_value());
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::AuxillaryTypeTrait{}));
        BOOST_TEST_REQUIRE(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::AuxillaryTypeTrait{})) == AuxillaryTraitsTypes::AuxillaryTypes::Pump);
    }

    {
        auto test_ptr = factory.OneTouchDevice_CreateDevice(Utility::AuxillaryState(valid_aux_label_and_status[7]));
        BOOST_TEST_REQUIRE(test_ptr.has_value());
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::AuxillaryTypeTrait{}));
        BOOST_TEST_REQUIRE(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::AuxillaryTypeTrait{})) == AuxillaryTraitsTypes::AuxillaryTypes::Spillover);
    }

    static const std::vector<std::string> invalid_aux_label_and_status =
    {
        "Auxillary     ON",
        "Aux E1       OFF",
        "Sprinkler     ON",
        "Extra Aux 2  OFF",
        "Aux11         ON"
    };

    for (auto bad_device_and_status : invalid_aux_label_and_status)
    {
        BOOST_TEST(!factory.OneTouchDevice_CreateDevice(Utility::AuxillaryState(bad_device_and_status)).has_value());
    }
}

BOOST_AUTO_TEST_SUITE_END()
