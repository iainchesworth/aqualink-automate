#include "kernel/auxillary_devices/stable_id.h"

#include <boost/uuid/name_generator_sha1.hpp>
#include <boost/uuid/string_generator.hpp>

namespace AqualinkAutomate::Kernel
{

	boost::uuids::uuid DeriveStableUuid(std::string_view native_key)
	{
		// Fixed project namespace UUID for all derived device identities. NEVER change this
		// value: doing so would change every derived id (and orphan every persisted cache /
		// external reference). Function-local static avoids any static-init-order concern.
		static const boost::uuids::uuid s_AqualinkDeviceNamespace =
			boost::uuids::string_generator{}("a4f5c2e1-9b3d-5f8a-bc12-7e0d1a2b3c4d");

		boost::uuids::name_generator_sha1 generator{ s_AqualinkDeviceNamespace };
		return generator(native_key.data(), native_key.size());
	}

}
// namespace AqualinkAutomate::Kernel
