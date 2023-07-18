#include <boost/test/unit_test.hpp>

#include "kernel/auxillary_traits.h"

BOOST_AUTO_TEST_SUITE(AuxillaryTraitsTestSuite)

BOOST_AUTO_TEST_CASE(TraitType_SetGet_Test)
{
    using namespace AqualinkAutomate::Kernel;

    using TestTypes = uint32_t;

    const TestTypes TestType_1 = 1;
    const TestTypes TestType_A = 2;

    class TestTypesTrait : public ImmutableTraitType<TestTypes>
    {
    public:
        virtual TraitKey Name() const final { return std::string{"TestTypesTrait"}; }
    };

    Traits traits;

    BOOST_REQUIRE(traits.Set(TestTypesTrait{}, TestType_1));

    auto get_test1_output = traits.TryGet(TestTypesTrait{});
    BOOST_REQUIRE(get_test1_output.has_value());

    TestTypes get_test2_output = get_test1_output.value();
    BOOST_CHECK_EQUAL(TestType_1, get_test2_output);
}

BOOST_AUTO_TEST_CASE(TraitType_SetGetOperators_Test)
{
    using namespace AqualinkAutomate::Kernel;

    using TestTypes = uint32_t;

    const TestTypes TestType_1 = 1;
    const TestTypes TestType_A = 2;

    class TestTypesTrait : public MutableTraitType<TestTypes>
    {
    public:
        virtual TraitKey Name() const final { return std::string{"TestTypesTrait"}; }
    };

    Traits traits;

    BOOST_REQUIRE(traits.Set(TestTypesTrait{}, TestType_1));

    TestTypes get_test1_output = traits[TestTypesTrait{}];
    BOOST_CHECK_EQUAL(TestType_1, get_test1_output);

    traits[TestTypesTrait{}] = TestType_A;
    TestTypes get_test2_output = traits[TestTypesTrait{}];
    BOOST_CHECK_EQUAL(TestType_A, get_test2_output);
}

BOOST_AUTO_TEST_CASE(TraitType_SetGetOperators_Throwing_Test)
{
    using namespace AqualinkAutomate::Kernel;

    using TestTypes = uint32_t;

    const TestTypes TestType_1 = 1;
    const TestTypes TestType_A = 2;

    class TestTypesTrait : public ImmutableTraitType<TestTypes>
    {
    public:
        virtual TraitKey Name() const final { return std::string{"TestTypesTrait"}; }
    };

    Traits traits;

    BOOST_REQUIRE_THROW(traits[TestTypesTrait{}], std::runtime_error);  // Trait does not exist.

    BOOST_REQUIRE(traits.Set(TestTypesTrait{}, TestType_1));
    BOOST_REQUIRE(!traits.Set(TestTypesTrait{}, TestType_A));  // Trait is immutable.

    TestTypes get_test1_output = traits[TestTypesTrait{}];
    BOOST_CHECK_EQUAL(TestType_1, get_test1_output);
}

BOOST_AUTO_TEST_CASE(TraitType_Has_Test)
{
    using namespace AqualinkAutomate::Kernel;

    using TestTypes = uint32_t;

    const TestTypes TestType_1 = 1;
    const TestTypes TestType_A = 2;

    class TestTypesTrait : public ImmutableTraitType<TestTypes>
    {
    public:
        virtual TraitKey Name() const final { return std::string{"TestTypesTrait"}; }
    };

    Traits traits;

    BOOST_REQUIRE(!traits.Has(TestTypesTrait{}));
    BOOST_REQUIRE(traits.Set(TestTypesTrait{}, TestType_1));
    BOOST_REQUIRE(traits.Has(TestTypesTrait{}));
}

BOOST_AUTO_TEST_CASE(TraitType_Remove_Test)
{
    using namespace AqualinkAutomate::Kernel;

    using TestTypes = uint32_t;

    const TestTypes TestType_1 = 1;
    const TestTypes TestType_A = 2;

    class TestTypesTrait : public ImmutableTraitType<TestTypes>
    {
    public:
        virtual TraitKey Name() const final { return std::string{"TestTypesTrait"}; }
    };

    Traits traits;

    BOOST_REQUIRE(!traits.Has(TestTypesTrait{}));
    
    BOOST_REQUIRE(traits.Set(TestTypesTrait{}, TestType_1));
    BOOST_REQUIRE(traits.Has(TestTypesTrait{}));
    
    traits.Remove(TestTypesTrait{});
    BOOST_REQUIRE(!traits.Has(TestTypesTrait{}));
}

BOOST_AUTO_TEST_SUITE_END()
