#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

#include <boost/uuid/uuid.hpp>

#include "kernel/auxillary_traits/auxillary_traits.h"
#include "kernel/auxillary_traits/auxillary_traits_types.h"

namespace AqualinkAutomate::Kernel
{

	enum class AuxillaryHealthStates
	{
		Healthy,
		Unhealthy,
		Unknown
	};
	
	class AuxillaryBase
	{
	public:
		virtual boost::uuids::uuid Id() const = 0;

	public:
		bool operator==(const AuxillaryBase& other) const;
		bool operator==(const boost::uuids::uuid id) const;
		bool operator==(const std::string& id) const;

	public:
		bool operator!=(const AuxillaryBase& other) const;

	public:
		Traits AuxillaryTraits{};

	public:
		AuxillaryHealthStates Health() const;
	};

	template<typename AUX_STATUSES>
	class AuxillaryBaseWithStatus : public AuxillaryBase
	{
	public:
		AuxillaryBaseWithStatus(const std::string& label, const AUX_STATUSES aux_statuses) :
			m_Status(aux_statuses)
		{
			AuxillaryTraits.Set(AuxillaryTraitsTypes::LabelTrait{}, label);
		}

	public:
		const std::string Label() const
		{
			// Assume that there's a label as it's only ever set above in the constructor.
			return AuxillaryTraits.TryGet(AuxillaryTraitsTypes::LabelTrait{}).value();
		}

		AUX_STATUSES Status() const
		{
			return m_Status;
		}

	public:
		virtual void Status(const AUX_STATUSES aux_status) = 0;

	protected:
		const std::string m_Label;
		AUX_STATUSES m_Status;
	};

}
// namespace AqualinkAutomate::Kernel
