#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <string>
#include <vector>

#include "exceptions/exception_traits_doesnotexist.h"
#include "exceptions/exception_traits_notmutable.h"
#include "kernel/auxillary_traits/auxillary_traits.h"
#include "kernel/auxillary_traits/auxillary_traits_base.h"
#include "kernel/auxillary_traits/auxillary_traits_proxy.h"

BOOST_AUTO_TEST_SUITE(AuxillaryTraitsTestSuite)

BOOST_AUTO_TEST_CASE(Test_MutableTraits_Simple)
{
    using namespace AqualinkAutomate::Kernel;

    using TestTypes_Simple = uint32_t;
    class TestTypesTrait_Simple : public MutableTraitType<TestTypes_Simple>
    {
    public:
        virtual TraitKey Name() const final { return std::string{"TestTypesTrait_Mutable_Simple"}; }
    };

    Traits traits;
    TestTypesTrait_Simple trait;

    // Check trait does not exist
    BOOST_CHECK(!traits.Has(trait));

    // Set trait
    traits.Set(trait, 1234);
    BOOST_CHECK(traits.Has(trait));

    // Get trait
    auto traitValue = *(traits.Get(trait));
    BOOST_CHECK(traitValue == 1234);

    // Update trait
    traits[trait] = 5678;
    traitValue = *(traits.Get(trait));
    BOOST_CHECK(traitValue == 5678);

    // Remove trait
    BOOST_CHECK(traits.Remove(trait));
    BOOST_CHECK(!traits.Has(trait));
}

BOOST_AUTO_TEST_CASE(Test_ImmutableTraits_Simple)
{
    using namespace AqualinkAutomate::Exceptions;
    using namespace AqualinkAutomate::Kernel;

    using TestTypes_Simple = uint32_t;
    class TestTypesTrait_Simple : public ImmutableTraitType<TestTypes_Simple>
    {
    public:
        virtual TraitKey Name() const final { return std::string{"TestTypesTrait_Immutable_Simple"}; }
    }; 
    
    Traits traits;
    TestTypesTrait_Simple trait;

    // Check trait does not exist
    BOOST_CHECK(!traits.Has(trait));

    // Set trait
    traits.Set(trait, 1234);
    BOOST_CHECK(traits.Has(trait));

    // Get trait
    auto traitValue = *(traits.Get(trait));
    BOOST_CHECK(traitValue == 1234);

    // Attempt to update trait - should not be permitted
    BOOST_CHECK_THROW(traits[trait] = 5678, Traits_NotMutable);

    // Attempt to remove trait - should not be permitted
    BOOST_CHECK(!traits.Remove(trait));
    BOOST_CHECK(traits.Has(trait));
}

BOOST_AUTO_TEST_CASE(Test_MutableTraits_Complex)
{
    using namespace AqualinkAutomate::Kernel;

    using TestTypes_Complex = std::vector<uint32_t>;
    class TestTypesTrait_Complex : public MutableTraitType<TestTypes_Complex>
    {
    public:
        virtual TraitKey Name() const final { return std::string{"TestTypesTrait_Mutable_Complex"}; }
    };

    Traits traits;    
    TestTypesTrait_Complex trait;
    std::vector<uint32_t> test_dataset_1{1,2,3,4,5};
    std::vector<uint32_t> test_dataset_2{6,7,8,9,10};

    // Check trait does not exist
    BOOST_CHECK(!traits.Has(trait));

    // Set trait
    traits.Set(trait, test_dataset_1);
    BOOST_CHECK(traits.Has(trait));

    // Get trait
    auto traitValue = *(traits.Get(trait));
    BOOST_CHECK(traitValue == test_dataset_1);

    // Update trait
    traits[trait] = test_dataset_2;
    traitValue = *(traits.Get(trait));
    BOOST_CHECK(traitValue == test_dataset_2);

    // Remove trait
    BOOST_CHECK(traits.Remove(trait));
    BOOST_CHECK(!traits.Has(trait));
}

BOOST_AUTO_TEST_CASE(Test_ImmutableTraits_Complex)
{
    using namespace AqualinkAutomate::Exceptions;
    using namespace AqualinkAutomate::Kernel;

    using TestTypes_Complex = std::vector<uint32_t>;
    class TestTypesTrait_Complex : public ImmutableTraitType<TestTypes_Complex>
    {
    public:
        virtual TraitKey Name() const final { return std::string{"TestTypesTrait_Immutable_Complex"}; }
    };

    Traits traits;
    TestTypesTrait_Complex trait;
    std::vector<uint32_t> test_dataset_1{1, 2, 3, 4, 5};
    std::vector<uint32_t> test_dataset_2{6, 7, 8, 9, 10};

    // Check trait does not exist
    BOOST_CHECK(!traits.Has(trait));

    // Set trait
    traits.Set(trait, test_dataset_1);
    BOOST_CHECK(traits.Has(trait));

    // Get trait
    auto traitValue = *(traits.Get(trait));
    BOOST_CHECK(traitValue == test_dataset_1);

    // Attempt to update trait - should not be permitted
    BOOST_CHECK_THROW(traits[trait] = test_dataset_2, Traits_NotMutable);

    // Attempt to remove trait - should not be permitted
    BOOST_CHECK(!traits.Remove(trait));
    BOOST_CHECK(traits.Has(trait));
}

BOOST_AUTO_TEST_CASE(Test_Traits_Get_Exceptions)
{
    using namespace AqualinkAutomate::Exceptions;
    using namespace AqualinkAutomate::Kernel;

    using TestTypes_Simple = uint32_t;
    class TestTypesTrait_Simple : public MutableTraitType<TestTypes_Simple>
    {
    public:
        virtual TraitKey Name() const final { return std::string{"TestTypesTrait_Simple"}; }
    }; 
    
    Traits traits;
    TestTypesTrait_Simple trait;

    // Attempt to get a trait that does not exist
    BOOST_CHECK_THROW(traits.Get(trait), Traits_DoesNotExist);
    // Attempt to get a trait through a TraitValueProxy that does not exist
    BOOST_CHECK_THROW(traits.Get(TestTypesTrait_Simple{}), Traits_DoesNotExist);

    // Add the trait
    traits.Set(trait, 1234);

    using TestTypes_Complex = std::vector<uint32_t>;
    class TestTypesTrait_Complex : public MutableTraitType<TestTypes_Complex>
    {
    public:
        virtual TraitKey Name() const final { return std::string{"TestTypesTrait_Complex"}; }
    };

    std::vector<uint32_t>::const_iterator cit;

    // Attempt to get a trait through a ConstTraitValueProxy that does not exist
    BOOST_CHECK_THROW(const auto& ABC = *(traits.Get(TestTypesTrait_Complex{})), Traits_DoesNotExist);
    BOOST_CHECK_THROW((*(traits.Get(TestTypesTrait_Complex{}))).reserve(1), Traits_DoesNotExist);
    BOOST_CHECK_THROW(traits.Get(TestTypesTrait_Complex{})->reserve(1), Traits_DoesNotExist);
}

BOOST_AUTO_TEST_SUITE_END()
