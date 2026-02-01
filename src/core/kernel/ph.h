#pragma once

#include <boost/cstdfloat.hpp>

namespace AqualinkAutomate::Kernel
{

	class pH
	{
		inline static const boost::float32_t MINIMUM_PH_VALUE = 0.0f;
		inline static const boost::float32_t MAXIMUM_PH_VALUE = 14.0f;

	public:
		pH(boost::float32_t value);

	public:
		boost::float32_t operator()() const;

	public:
		pH& operator=(boost::float32_t value);

	public:
		bool operator==(const pH& other) const;

	private:
		static boost::float32_t ClampAndRound(boost::float32_t value);

	private:
		boost::float32_t m_pH;
	};

}
// namespace AqualinkAutomate::Kernel
