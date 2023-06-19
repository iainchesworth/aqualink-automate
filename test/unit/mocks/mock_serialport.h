#pragma once 

#include "developer/mock_serial_port.h"

using namespace AqualinkAutomate;
using namespace AqualinkAutomate::Developer;

namespace AqualinkAutomate::Test
{

	class MockSerialPort : public Developer::mock_serial_port
	{
	};

}
// namespace AqualinkAutomate::Test
