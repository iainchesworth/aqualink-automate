#pragma once

#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

#include <boost/system/error_code.hpp>
#include <boost/uuid/uuid.hpp>

#include "kernel/auxillary_traits.h"

namespace AqualinkAutomate::Kernel
{
	class AuxillaryBase
	{
	public:
		virtual boost::uuids::uuid Id() const = 0;

	public:
		bool operator==(const AuxillaryBase& other) const;
		bool operator==(const boost::uuids::uuid id) const;
		bool operator==(const std::string& id) const;

	public:
		Traits AuxillaryTraits{};
		std::set<boost::system::error_code> ErrorCodes{};
	};

	template<typename AUX_STATUSES>
	class AuxillaryBaseWithStatus : public AuxillaryBase
	{
	public:
		AuxillaryBaseWithStatus(const std::string& label, const AUX_STATUSES aux_statuses) :
			m_Label(label),
			m_Status(aux_statuses)
		{
		}

	public:
		bool operator==(const AuxillaryBaseWithStatus<AUX_STATUSES>& other) const
		{
			return (m_Label == other.m_Label);
		}

		bool operator!=(const AuxillaryBaseWithStatus<AUX_STATUSES>& other) const
		{
			return !operator==(other);
		}

	public:
		const std::string& Label() const
		{
			return m_Label;
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
