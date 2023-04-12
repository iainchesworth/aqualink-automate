#include "equipment/jandy/jandy_equipment.h"

namespace AqualinkAutomate::Equipment::Jandy
{
	JandyEquipment::JandyEquipment(Serial::SerialPort& serial_port) : EquipmentBase(serial_port)
	{
	}
}
// namespace AqualinkAutomate::Equipment::Jandy
