#include <memory>
#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/uuid/uuid.hpp>

#include "jandy/auxillaries/jandy_auxillary_id.h"
#include "jandy/auxillaries/jandy_auxillary_reconciliation.h"
#include "jandy/auxillaries/jandy_auxillary_traits_types.h"
#include "kernel/auxillary_devices/auxillary_device.h"
#include "kernel/auxillary_devices/stable_id.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"
#include "kernel/device_graph/device_graph.h"

using namespace AqualinkAutomate;
namespace Traits = Kernel::AuxillaryTraitsTypes;

//=============================================================================
// ParseAuxId - tolerant of no-space/space bank forms + whitespace artefacts.
//=============================================================================

BOOST_AUTO_TEST_SUITE(ParseAuxId_TestSuite)

BOOST_AUTO_TEST_CASE(AcceptsCanonicalAndVariantSpellings)
{
	// Bank A (no space canonically) - accept with or without a space.
	BOOST_CHECK(Auxillaries::ParseAuxId("Aux1") == Auxillaries::JandyAuxillaryIds::Aux_1);
	BOOST_CHECK(Auxillaries::ParseAuxId("Aux5") == Auxillaries::JandyAuxillaryIds::Aux_5);
	BOOST_CHECK(Auxillaries::ParseAuxId("Aux 5") == Auxillaries::JandyAuxillaryIds::Aux_5);
	BOOST_CHECK(Auxillaries::ParseAuxId("Aux7") == Auxillaries::JandyAuxillaryIds::Aux_7);

	// Banks B-D (space canonically) - accept with, without, or doubled spacing.
	BOOST_CHECK(Auxillaries::ParseAuxId("Aux B1") == Auxillaries::JandyAuxillaryIds::Aux_B1);
	BOOST_CHECK(Auxillaries::ParseAuxId("AuxB1") == Auxillaries::JandyAuxillaryIds::Aux_B1);
	BOOST_CHECK(Auxillaries::ParseAuxId("Aux  B1") == Auxillaries::JandyAuxillaryIds::Aux_B1);
	BOOST_CHECK(Auxillaries::ParseAuxId("  Aux B1  ") == Auxillaries::JandyAuxillaryIds::Aux_B1);
	BOOST_CHECK(Auxillaries::ParseAuxId("Aux D8") == Auxillaries::JandyAuxillaryIds::Aux_D8);
	BOOST_CHECK(Auxillaries::ParseAuxId("AuxD8") == Auxillaries::JandyAuxillaryIds::Aux_D8);

	// Extra Aux, with or without the interior space.
	BOOST_CHECK(Auxillaries::ParseAuxId("Extra Aux") == Auxillaries::JandyAuxillaryIds::ExtraAux);
	BOOST_CHECK(Auxillaries::ParseAuxId("ExtraAux") == Auxillaries::JandyAuxillaryIds::ExtraAux);
}

BOOST_AUTO_TEST_CASE(RejectsNonAuxAndOutOfRange)
{
	BOOST_CHECK(!Auxillaries::ParseAuxId("").has_value());
	BOOST_CHECK(!Auxillaries::ParseAuxId("pool light").has_value());
	BOOST_CHECK(!Auxillaries::ParseAuxId("Pool Light").has_value());
	BOOST_CHECK(!Auxillaries::ParseAuxId("Swim Jet").has_value());
	BOOST_CHECK(!Auxillaries::ParseAuxId("Auxillary").has_value());
	BOOST_CHECK(!Auxillaries::ParseAuxId("Sprinkler").has_value());
	BOOST_CHECK(!Auxillaries::ParseAuxId("Aux11").has_value());      // two digits
	BOOST_CHECK(!Auxillaries::ParseAuxId("Aux0").has_value());       // below range
	BOOST_CHECK(!Auxillaries::ParseAuxId("Aux8").has_value());       // bank A max is 7
	BOOST_CHECK(!Auxillaries::ParseAuxId("Aux E1").has_value());     // no bank E
	BOOST_CHECK(!Auxillaries::ParseAuxId("Aux B9").has_value());     // bank max is 8
	BOOST_CHECK(!Auxillaries::ParseAuxId("Extra Aux 2").has_value());
}

BOOST_AUTO_TEST_SUITE_END()

//=============================================================================
// Stable identity - deterministic, collision-free per aux id.
//=============================================================================

BOOST_AUTO_TEST_SUITE(AuxStableId_TestSuite)

BOOST_AUTO_TEST_CASE(NativeKeyIsCanonicalIntegerKeyed)
{
	BOOST_CHECK_EQUAL(Auxillaries::AuxNativeKey(Auxillaries::JandyAuxillaryIds::Aux_5), "jandy:aux:5");
	BOOST_CHECK_EQUAL(Auxillaries::AuxNativeKey(Auxillaries::JandyAuxillaryIds::Aux_B1), "jandy:aux:8");
	BOOST_CHECK_EQUAL(Auxillaries::AuxNativeKey(Auxillaries::JandyAuxillaryIds::ExtraAux), "jandy:aux:0");
}

BOOST_AUTO_TEST_CASE(StableIdIsDeterministicAndDistinct)
{
	const auto a1 = Auxillaries::AuxStableId(Auxillaries::JandyAuxillaryIds::Aux_5);
	const auto a2 = Auxillaries::AuxStableId(Auxillaries::JandyAuxillaryIds::Aux_5);
	const auto b = Auxillaries::AuxStableId(Auxillaries::JandyAuxillaryIds::Aux_6);

	BOOST_CHECK(a1 == a2);                              // same aux id -> same id
	BOOST_CHECK(a1 != b);                               // different aux id -> different id
	BOOST_CHECK(!a1.is_nil());
	// All spelling variants resolve to the SAME stable id (via the same aux id).
	BOOST_CHECK(a1 == Auxillaries::AuxStableId(Auxillaries::ParseAuxId("Aux 5").value()));
	// Equivalent to deriving directly from the native key.
	BOOST_CHECK(a1 == Kernel::DeriveStableUuid("jandy:aux:5"));
}

BOOST_AUTO_TEST_CASE(DeriveStableUuidIsStableAcrossCalls)
{
	BOOST_CHECK(Kernel::DeriveStableUuid("jandy:aux:5") == Kernel::DeriveStableUuid("jandy:aux:5"));
	BOOST_CHECK(Kernel::DeriveStableUuid("jandy:aux:5") != Kernel::DeriveStableUuid("jandy:aux:6"));
}

BOOST_AUTO_TEST_SUITE_END()

//=============================================================================
// Reconciliation helpers - identity adoption + legacy-orphan pruning.
//=============================================================================

namespace
{
	std::shared_ptr<Kernel::AuxillaryDevice> MakeAux(const std::string& label, bool with_aux_id, Auxillaries::JandyAuxillaryIds id)
	{
		auto dev = with_aux_id
			? std::make_shared<Kernel::AuxillaryDevice>(Auxillaries::AuxStableId(id))
			: std::make_shared<Kernel::AuxillaryDevice>();
		dev->AuxillaryTraits.Set(Traits::AuxillaryTypeTrait{}, Traits::AuxillaryTypes::Auxillary);
		dev->AuxillaryTraits.Set(Traits::LabelTrait{}, label);
		if (with_aux_id)
		{
			dev->AuxillaryTraits.Set(Auxillaries::JandyAuxillaryId{}, id);
		}
		return dev;
	}
}

BOOST_AUTO_TEST_SUITE(AuxReconciliation_TestSuite)

BOOST_AUTO_TEST_CASE(EnsureAuxIdentityGrantsIdentityAndIsIdempotent)
{
	// A cache-restored placeholder: label only, no aux id / hardware label.
	auto phantom = std::make_shared<Kernel::AuxillaryDevice>(Auxillaries::AuxStableId(Auxillaries::JandyAuxillaryIds::Aux_5));
	phantom->AuxillaryTraits.Set(Traits::LabelTrait{}, "Swim Jet");

	BOOST_REQUIRE(!phantom->AuxillaryTraits.Has(Auxillaries::JandyAuxillaryId{}));

	Auxillaries::EnsureAuxIdentity(phantom, Auxillaries::JandyAuxillaryIds::Aux_5);

	BOOST_REQUIRE(phantom->AuxillaryTraits.Has(Auxillaries::JandyAuxillaryId{}));
	BOOST_CHECK(*(phantom->AuxillaryTraits[Auxillaries::JandyAuxillaryId{}]) == Auxillaries::JandyAuxillaryIds::Aux_5);
	BOOST_REQUIRE(phantom->AuxillaryTraits.Has(Traits::HardwareLabelTrait{}));
	BOOST_CHECK_EQUAL(std::string{ *(phantom->AuxillaryTraits[Traits::HardwareLabelTrait{}]) }, "Aux5");

	// Calling again must NOT throw (HardwareLabelTrait is immutable; the guard prevents a re-set).
	BOOST_CHECK_NO_THROW(Auxillaries::EnsureAuxIdentity(phantom, Auxillaries::JandyAuxillaryIds::Aux_5));
}

BOOST_AUTO_TEST_CASE(RemoveOrphanAuxPlaceholdersDropsOnlyTheLabelOnlyPlaceholder)
{
	Kernel::DevicesGraph devices;

	// The legacy cache placeholder: type Auxillary, label "Swim Jet", but no aux id.
	auto orphan = MakeAux("Swim Jet", /*with_aux_id=*/false, Auxillaries::JandyAuxillaryIds::Aux_1);
	// The live device starts with its generic label (so Add's type+label dedup lets it in
	// alongside the placeholder), then live discovery relabels it - exactly how the real
	// duplicate arises.
	auto live = MakeAux("Aux1", /*with_aux_id=*/true, Auxillaries::JandyAuxillaryIds::Aux_1);
	auto other = MakeAux("Spa Jet", /*with_aux_id=*/false, Auxillaries::JandyAuxillaryIds::Aux_2);

	devices.Add(orphan);
	devices.Add(live);
	devices.Add(other);
	live->AuxillaryTraits.Set(Traits::LabelTrait{}, std::string{ "Swim Jet" });

	// Two "Swim Jet" now exist (placeholder + relabelled live) - the duplicate state.
	BOOST_REQUIRE_EQUAL(devices.CountByLabel("Swim Jet"), 2u);

	Auxillaries::RemoveOrphanAuxPlaceholders(devices, "Swim Jet", live);

	// The label-only placeholder is gone; the identified live device is kept.
	BOOST_CHECK_EQUAL(devices.CountByLabel("Swim Jet"), 1u);
	BOOST_CHECK_EQUAL(devices.CountByLabel("Spa Jet"), 1u);   // a different label is untouched
}

BOOST_AUTO_TEST_SUITE_END()
