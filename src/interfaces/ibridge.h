#pragma once

namespace AqualinkAutomate::Interfaces
{

	template<typename MESSAGE_TYPE>
	class IBridge
	{
	public:
		IBridge()
		{
		}

		virtual ~IBridge()
		{
		}

	public:
		virtual void Notify(const MESSAGE_TYPE& message) = 0;
	};

}
// namespace AqualinkAutomate::Interfaces
