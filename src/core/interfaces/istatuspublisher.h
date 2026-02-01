#pragma once

#include <concepts>
#include <memory>

#include <boost/signals2.hpp>

#include "interfaces/istatus.h"

namespace AqualinkAutomate::Interfaces
{

	class IStatusPublisher
	{
	public:
		using StatusType = std::weak_ptr<const Interfaces::IStatus>;
		using StatusSignalType = boost::signals2::signal<void(StatusType)>;

	public:
		IStatusPublisher() = delete;
		~IStatusPublisher() = default;

	public:
		template<typename DEVICE_STATUS>
			requires std::derived_from<DEVICE_STATUS, Interfaces::IStatus>
		IStatusPublisher(const DEVICE_STATUS& status)
		{
			Status(status);
		}

	public:
		StatusSignalType StatusSignal;

	public:
		StatusType Status() const;

	public:
		template<typename DEVICE_STATUS>
			requires std::derived_from<DEVICE_STATUS, Interfaces::IStatus>
		void Status(const DEVICE_STATUS& status)
		{
			m_Status = std::make_shared<DEVICE_STATUS>(status);			
			StatusSignal(StatusType(m_Status));
		}

	private:
		std::shared_ptr<const Interfaces::IStatus> m_Status{ nullptr };
	};

	//---------------------------------------------------------------------
	// STATUS COMPARE FUNCTIONS
	//---------------------------------------------------------------------

	// To support status publishing, there needs to be a mechanism to compare statuses

	template<typename THIS_STATUS, typename THAT_STATUS>
		requires std::derived_from<THIS_STATUS, Interfaces::IStatus>&& std::derived_from<THAT_STATUS, Interfaces::IStatus>
	bool operator==(const THIS_STATUS&, const THAT_STATUS&)
	{
		return std::same_as<std::remove_cvref_t<THIS_STATUS>, std::remove_cvref_t<THAT_STATUS>>;
	}

	template<typename THIS_STATUS, typename THAT_STATUS>
		requires std::derived_from<THIS_STATUS, Interfaces::IStatus>&& std::derived_from<THAT_STATUS, Interfaces::IStatus>
	bool operator!=(const THIS_STATUS& this_status, const THAT_STATUS& that_status)
	{
		return !(operator==(this_status, that_status));
	}

	// Enable comparisions of the std::weak_ptr type with a status type.

	template<typename THIS_STATUS>
		requires std::derived_from<THIS_STATUS, Interfaces::IStatus>
	bool operator==(const THIS_STATUS&, IStatusPublisher::StatusType that_status)
	{
		using THAT_STATUS = decltype(that_status)::element_type;

		return std::same_as<std::remove_cvref_t<THIS_STATUS>, std::remove_cvref_t<THAT_STATUS>>;
	}

}
// namespace AqualinkAutomate::Interfaces
