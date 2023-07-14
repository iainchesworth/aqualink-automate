#include <boost/test/unit_test.hpp>

#include "concepts/is_c_array.h"

using namespace AqualinkAutomate::Concepts;

BOOST_AUTO_TEST_SUITE(ConceptsTestSuite)

BOOST_AUTO_TEST_CASE(SatisfiesCArrayConcept) 
{
    int a[5];
    BOOST_CHECK((CArray<decltype(a)>));
}

BOOST_AUTO_TEST_CASE(DoesNotSatisfyCArrayConcept) 
{
    int a = 5;
    BOOST_CHECK(!(CArray<decltype(a)>));
}

BOOST_AUTO_TEST_CASE(VectorDoesNotSatisfyCArrayConcept) 
{
    std::vector<int> v;
    BOOST_CHECK(!(CArray<decltype(v)>));
}

BOOST_AUTO_TEST_CASE(SatisfiesCArrayRefConcept) 
{
    int a[5];
    BOOST_CHECK((CArrayRef<decltype(a)&>));
}

BOOST_AUTO_TEST_CASE(DoesNotSatisfyCArrayRefConcept)
{
    int a = 5;
    BOOST_CHECK(!(CArrayRef<decltype(a)&>));
}

BOOST_AUTO_TEST_SUITE_END()
