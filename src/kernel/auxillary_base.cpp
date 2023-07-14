#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include "kernel/auxillary_base.h"

namespace AqualinkAutomate::Kernel
{

	bool AuxillaryBase::operator==(const AuxillaryBase& other) const
	{
		return (Id() == other.Id());
	}

	bool AuxillaryBase::operator==(const boost::uuids::uuid id) const
	{
		return (Id() == id);
	}

	bool AuxillaryBase::operator==(const std::string& id) const
	{
		try
		{
			boost::uuids::string_generator gen;
			boost::uuids::uuid uuid = gen(id);

			return (Id() == uuid);
		}
		catch (const std::runtime_error& ex_re)
		{
			// The string is invalid did not decode into a UUID correctly.
			return false;
		}
	}

}
// namespace AqualinkAutomate::Kernel
