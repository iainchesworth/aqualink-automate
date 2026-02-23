#include "interfaces/istatuspublisher.h"

namespace AqualinkAutomate::Interfaces
{

	IStatusPublisher::StatusType IStatusPublisher::Status() const
	{
		return { m_Status };
	}

}
// namespace AqualinkAutomate::Interfaces
