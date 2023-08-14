#pragma once

#include <memory>
#include <vector>

#include <boost/asio/io_context.hpp>

#include "interfaces/idevice.h"
#include "interfaces/ideviceidentifier.h"

namespace AqualinkAutomate::Interfaces
{
	class IEquipment
	{
	public:
		IEquipment(boost::asio::io_context& io_context);
		virtual ~IEquipment();

	protected:
		void AddDevice(Interfaces::IDevice&& device);
		void AddDevice(std::unique_ptr<Interfaces::IDevice>&& device);

	protected:
		bool IsDeviceRegistered(const Interfaces::IDeviceIdentifier& device_id) const;

	protected:
		boost::asio::io_context& m_IOContext;

	private:
		std::vector<std::unique_ptr<Interfaces::IDevice>> m_Devices;
	};
}
// namespace AqualinkAutomate::Interfaces
