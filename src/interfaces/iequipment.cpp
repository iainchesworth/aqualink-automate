#include "interfaces/iequipment.h"

namespace AqualinkAutomate::Interfaces
{

	IEquipment::IEquipment(boost::asio::io_context& io_context) :
		m_IOContext(io_context)
	{
	}

	IEquipment::~IEquipment()
	{
	}

}
// namespace AqualinkAutomate::Interfaces
