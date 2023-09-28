#include "interfaces/iequipment.h"

namespace AqualinkAutomate::Interfaces
{

	IEquipment::IEquipment(Types::ExecutorType executor) :
		m_Executor(executor)
	{
	}

	IEquipment::~IEquipment()
	{
	}

}
// namespace AqualinkAutomate::Interfaces
