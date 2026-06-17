#pragma once

#include <string_view>

#include <boost/uuid/uuid.hpp>

namespace AqualinkAutomate::Kernel
{

	// Deterministically derive a stable device UUID (RFC 4122 v5, name-based SHA-1) from a
	// protocol-native key such as "jandy:aux:5".
	//
	// The same key always yields the same UUID, so a cache-restored device and a
	// live-discovered device for the SAME physical hardware reconcile to one identity -
	// across process restarts AND fresh installs (no random component). This is the
	// reconciliation key that lets the equipment cache (which cannot see protocol-specific
	// traits like the Jandy aux id) stay consistent with live discovery.
	//
	// The namespace is a fixed project constant; callers supply a namespaced, protocol-
	// scoped key (e.g. "jandy:aux:5", "pentair:pump:60") to avoid cross-protocol collisions.
	boost::uuids::uuid DeriveStableUuid(std::string_view native_key);

}
// namespace AqualinkAutomate::Kernel
