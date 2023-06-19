#pragma once

#include <string>

#include <boost/uuid/uuid.hpp>

namespace AqualinkAutomate::Config
{
	class AuxillaryBase
	{
	public:
		virtual boost::uuids::uuid Id() const = 0;
	};

	template<typename AUX_STATES>
	class AuxillaryBaseWithState : public AuxillaryBase
	{
	public:
		AuxillaryBaseWithState(const std::string& label, const AUX_STATES aux_state) :
			m_Label(label),
			m_State(aux_state)
		{
		}

	public:
		bool operator==(const AuxillaryBaseWithState<AUX_STATES>& other) const
		{
			return (m_Label == other.m_Label);
		}

		bool operator!=(const AuxillaryBaseWithState<AUX_STATES>& other) const
		{
			return !operator==(other);
		}

	public:
		const std::string& Label() const
		{
			return m_Label;
		}

		AUX_STATES State() const
		{
			return m_State;
		}

	protected:
		const std::string m_Label;
		AUX_STATES m_State;
	};

}
// namespace AqualinkAutomate::Config
