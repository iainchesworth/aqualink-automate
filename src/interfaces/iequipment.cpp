#include "interfaces/iequipment.h"

namespace AqualinkAutomate::Interfaces
{

	IEquipment::IEquipment(Types::ExecutorType executor) :
		m_Executor(std::move(executor))
	{
	}

}
// namespace AqualinkAutomate::Interfaces
