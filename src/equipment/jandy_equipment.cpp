#include "equipment/jandy_equipment.h"

namespace AqualinkAutomate::Equipment
{
	JandyEquipment::JandyEquipment(Serial::SerialPort& serial_port) : EquipmentBase(serial_port)
	{
	}
}
// namespace AqualinkAutomate::Equipment
