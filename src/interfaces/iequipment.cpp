#include "interfaces/iequipment.h"

namespace AqualinkAutomate::Interfaces
{

	IEquipment::IEquipment(boost::asio::io_context& io_context) :
		m_IOContext(io_context),
		m_Devices()
	{
	}

	IEquipment::~IEquipment()
	{
	}

	void IEquipment::AddDevice(Interfaces::IDevice&& device)
	{
		m_Devices.push_back(std::move(std::make_unique<Interfaces::IDevice>(std::move(device))));
	}

	void IEquipment::AddDevice(std::unique_ptr<Interfaces::IDevice>&& device)
	{
		m_Devices.push_back(std::move(device));
	}

	bool IEquipment::IsDeviceRegistered(const Interfaces::IDeviceIdentifier& device_id) const
	{
		auto const& device_it = std::find_if(m_Devices.cbegin(), m_Devices.cend(), [&](const auto& device)
			{
				return ((nullptr != device) && (device->DeviceId() == device_id));
			}
		);

		return (m_Devices.cend() != device_it);
	}

}
// namespace AqualinkAutomate::Interfaces
