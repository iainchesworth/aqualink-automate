#include <tuple>

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

    const std::vector<Auxillaries::JandyAuxillaryIds> valid_aux_ids =
    {
        Auxillaries::JandyAuxillaryIds::Aux_1,
        Auxillaries::JandyAuxillaryIds::Aux_B1,
        Auxillaries::JandyAuxillaryIds::ExtraAux
    };
    
    {
        auto test_ptr = factory.SerialAdapterDevice_CreateDevice(valid_aux_ids[0]);
        BOOST_TEST_REQUIRE(test_ptr.has_value());
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::AuxillaryTypeTrait{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::AuxillaryTypeTrait{})) == AuxillaryTraitsTypes::AuxillaryTypes::Auxillary);
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(Auxillaries::JandyAuxillaryId{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(Auxillaries::JandyAuxillaryId{})) == Auxillaries::JandyAuxillaryIds::Aux_1);
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::AuxillaryStatusTrait{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::AuxillaryStatusTrait{})) == Kernel::AuxillaryStatuses::Unknown);
    }

    {
        auto test_ptr = factory.SerialAdapterDevice_CreateDevice(valid_aux_ids[1]);
        BOOST_TEST_REQUIRE(test_ptr.has_value());
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::AuxillaryTypeTrait{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::AuxillaryTypeTrait{})) == AuxillaryTraitsTypes::AuxillaryTypes::Auxillary);
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(Auxillaries::JandyAuxillaryId{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(Auxillaries::JandyAuxillaryId{})) == Auxillaries::JandyAuxillaryIds::Aux_B1);
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::AuxillaryStatusTrait{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::AuxillaryStatusTrait{})) == Kernel::AuxillaryStatuses::Unknown);
    }

    {
        auto test_ptr = factory.SerialAdapterDevice_CreateDevice(valid_aux_ids[2]);
        BOOST_TEST_REQUIRE(test_ptr.has_value());
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::AuxillaryTypeTrait{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::AuxillaryTypeTrait{})) == AuxillaryTraitsTypes::AuxillaryTypes::Auxillary);
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(Auxillaries::JandyAuxillaryId{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(Auxillaries::JandyAuxillaryId{})) == Auxillaries::JandyAuxillaryIds::ExtraAux);
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::AuxillaryStatusTrait{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::AuxillaryStatusTrait{})) == Kernel::AuxillaryStatuses::Unknown);
    }

    const std::vector<std::tuple<Auxillaries::JandyAuxillaryIds, Auxillaries::JandyAuxillaryStatuses>> valid_aux_ids_with_status =
    {
        { Auxillaries::JandyAuxillaryIds::Aux_1, Auxillaries::JandyAuxillaryStatuses::Off },
        { Auxillaries::JandyAuxillaryIds::Aux_B1, Auxillaries::JandyAuxillaryStatuses::On },
        { Auxillaries::JandyAuxillaryIds::ExtraAux, Auxillaries::JandyAuxillaryStatuses::Unknown }
    };

    {
        auto test_ptr = factory.SerialAdapterDevice_CreateDevice(std::get<Auxillaries::JandyAuxillaryIds>(valid_aux_ids_with_status[0]), std::get<Auxillaries::JandyAuxillaryStatuses>(valid_aux_ids_with_status[0]));
        BOOST_TEST_REQUIRE(test_ptr.has_value());
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::AuxillaryTypeTrait{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::AuxillaryTypeTrait{})) == AuxillaryTraitsTypes::AuxillaryTypes::Auxillary);
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(Auxillaries::JandyAuxillaryId{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(Auxillaries::JandyAuxillaryId{})) == Auxillaries::JandyAuxillaryIds::Aux_1);
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::AuxillaryStatusTrait{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::AuxillaryStatusTrait{})) == Kernel::AuxillaryStatuses::Off);
    }

    {
        auto test_ptr = factory.SerialAdapterDevice_CreateDevice(std::get<Auxillaries::JandyAuxillaryIds>(valid_aux_ids_with_status[1]), std::get<Auxillaries::JandyAuxillaryStatuses>(valid_aux_ids_with_status[1]));
        BOOST_TEST_REQUIRE(test_ptr.has_value());
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::AuxillaryTypeTrait{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::AuxillaryTypeTrait{})) == AuxillaryTraitsTypes::AuxillaryTypes::Auxillary);
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(Auxillaries::JandyAuxillaryId{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(Auxillaries::JandyAuxillaryId{})) == Auxillaries::JandyAuxillaryIds::Aux_B1);
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::AuxillaryStatusTrait{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::AuxillaryStatusTrait{})) == Kernel::AuxillaryStatuses::On);
    }

    {
        auto test_ptr = factory.SerialAdapterDevice_CreateDevice(std::get<Auxillaries::JandyAuxillaryIds>(valid_aux_ids_with_status[2]), std::get<Auxillaries::JandyAuxillaryStatuses>(valid_aux_ids_with_status[2]));
        BOOST_TEST_REQUIRE(test_ptr.has_value());
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::AuxillaryTypeTrait{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::AuxillaryTypeTrait{})) == AuxillaryTraitsTypes::AuxillaryTypes::Auxillary);
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(Auxillaries::JandyAuxillaryId{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(Auxillaries::JandyAuxillaryId{})) == Auxillaries::JandyAuxillaryIds::ExtraAux);
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::AuxillaryStatusTrait{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::AuxillaryStatusTrait{})) == Kernel::AuxillaryStatuses::Unknown);
    }

}

BOOST_AUTO_TEST_CASE(OneTouchDevice_CreateDevice_Test) 
{
    Factory::JandyAuxillaryFactory& factory = Factory::JandyAuxillaryFactory::Instance();

    const std::vector<std::string> valid_aux_label_and_status =
    {
        "Aux1         OFF",
        "Aux B1        ON",
        "Extra Aux    OFF",
        "AquaPure      ON",
        "Cleaner      OFF",
        "Heat Pump    OFF",
        "Pool Pump     ON",
        "Spillover    OFF",
        "Spa Heat     ENA"
    };

    {
        auto test_ptr = factory.OneTouchDevice_CreateDevice(Utility::AuxillaryState(valid_aux_label_and_status[0]));
        BOOST_TEST_REQUIRE(test_ptr.has_value());
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::AuxillaryTypeTrait{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::AuxillaryTypeTrait{})) == AuxillaryTraitsTypes::AuxillaryTypes::Auxillary);
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(Auxillaries::JandyAuxillaryId{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(Auxillaries::JandyAuxillaryId{})) == Auxillaries::JandyAuxillaryIds::Aux_1);
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::AuxillaryStatusTrait{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::AuxillaryStatusTrait{})) == Kernel::AuxillaryStatuses::Off);

    }

    {
        auto test_ptr = factory.OneTouchDevice_CreateDevice(Utility::AuxillaryState(valid_aux_label_and_status[1]));
        BOOST_TEST_REQUIRE(test_ptr.has_value());
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::AuxillaryTypeTrait{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::AuxillaryTypeTrait{})) == AuxillaryTraitsTypes::AuxillaryTypes::Auxillary);
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(Auxillaries::JandyAuxillaryId{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(Auxillaries::JandyAuxillaryId{})) == Auxillaries::JandyAuxillaryIds::Aux_B1);
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::AuxillaryStatusTrait{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::AuxillaryStatusTrait{})) == Kernel::AuxillaryStatuses::On);
    }

    {
        auto test_ptr = factory.OneTouchDevice_CreateDevice(Utility::AuxillaryState(valid_aux_label_and_status[2]));
        BOOST_TEST_REQUIRE(test_ptr.has_value());
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::AuxillaryTypeTrait{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::AuxillaryTypeTrait{})) == AuxillaryTraitsTypes::AuxillaryTypes::Auxillary);
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(Auxillaries::JandyAuxillaryId{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(Auxillaries::JandyAuxillaryId{})) == Auxillaries::JandyAuxillaryIds::ExtraAux);
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::AuxillaryStatusTrait{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::AuxillaryStatusTrait{})) == Kernel::AuxillaryStatuses::Off);
    }
    
    {
        auto test_ptr = factory.OneTouchDevice_CreateDevice(Utility::AuxillaryState(valid_aux_label_and_status[3]));
        BOOST_TEST_REQUIRE(test_ptr.has_value());
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::AuxillaryTypeTrait{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::AuxillaryTypeTrait{})) == AuxillaryTraitsTypes::AuxillaryTypes::Chlorinator);
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::ChlorinatorStatusTrait{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::ChlorinatorStatusTrait{})) == Kernel::ChlorinatorStatuses::Running);
    }

    {
        auto test_ptr = factory.OneTouchDevice_CreateDevice(Utility::AuxillaryState(valid_aux_label_and_status[4]));
        BOOST_TEST_REQUIRE(test_ptr.has_value());
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::AuxillaryTypeTrait{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::AuxillaryTypeTrait{})) == AuxillaryTraitsTypes::AuxillaryTypes::Cleaner);
    }

    {
        auto test_ptr = factory.OneTouchDevice_CreateDevice(Utility::AuxillaryState(valid_aux_label_and_status[5]));
        BOOST_TEST_REQUIRE(test_ptr.has_value());
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::AuxillaryTypeTrait{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::AuxillaryTypeTrait{})) == AuxillaryTraitsTypes::AuxillaryTypes::Heater);
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::HeaterStatusTrait{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::HeaterStatusTrait{})) == Kernel::HeaterStatuses::Off);
    }

    {
        auto test_ptr = factory.OneTouchDevice_CreateDevice(Utility::AuxillaryState(valid_aux_label_and_status[6]));
        BOOST_TEST_REQUIRE(test_ptr.has_value());
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::AuxillaryTypeTrait{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::AuxillaryTypeTrait{})) == AuxillaryTraitsTypes::AuxillaryTypes::Pump);
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::PumpStatusTrait{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::PumpStatusTrait{})) == Kernel::PumpStatuses::Running);
    }

    {
        auto test_ptr = factory.OneTouchDevice_CreateDevice(Utility::AuxillaryState(valid_aux_label_and_status[7]));
        BOOST_TEST_REQUIRE(test_ptr.has_value());
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::AuxillaryTypeTrait{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::AuxillaryTypeTrait{})) == AuxillaryTraitsTypes::AuxillaryTypes::Spillover);
    }

    {
        auto test_ptr = factory.OneTouchDevice_CreateDevice(Utility::AuxillaryState(valid_aux_label_and_status[8]));
        BOOST_TEST_REQUIRE(test_ptr.has_value());
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::AuxillaryTypeTrait{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::AuxillaryTypeTrait{})) == AuxillaryTraitsTypes::AuxillaryTypes::Heater);
        BOOST_TEST_REQUIRE(test_ptr.value()->AuxillaryTraits.Has(AuxillaryTraitsTypes::HeaterStatusTrait{}));
        BOOST_TEST(*(test_ptr.value()->AuxillaryTraits.Get(AuxillaryTraitsTypes::HeaterStatusTrait{})) == Kernel::HeaterStatuses::Enabled);
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
