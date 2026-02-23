#include "kernel/body_of_water.h"

namespace AqualinkAutomate::Kernel
{

	BodyOfWater::BodyOfWater(BodyOfWaterIds id, std::string_view label)
		: m_Id(id)
		, m_Label(label)
	{
	}

	BodyOfWaterIds BodyOfWater::Id() const
	{
		return m_Id;
	}

	const std::string& BodyOfWater::Label() const
	{
		return m_Label;
	}

	bool BodyOfWater::IsActive() const
	{
		return m_IsActive;
	}

	void BodyOfWater::IsActive(bool active)
	{
		m_IsActive = active;
	}

	std::optional<Temperature> BodyOfWater::CurrentTemp() const
	{
		return m_CurrentTemp;
	}

	void BodyOfWater::CurrentTemp(const Temperature& temp)
	{
		m_CurrentTemp = temp;
	}

	std::optional<Temperature> BodyOfWater::TempSetpoint() const
	{
		return m_TempSetpoint;
	}

	void BodyOfWater::TempSetpoint(const Temperature& setpoint)
	{
		m_TempSetpoint = setpoint;
	}

}
// namespace AqualinkAutomate::Kernel
