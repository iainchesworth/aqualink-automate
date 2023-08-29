#pragma once

#include <concepts>
#include <memory>
#include <string_view>
#include <type_traits>

#include <boost/signals2.hpp>

namespace AqualinkAutomate::Interfaces
{

	class IStatus
	{
	public:
		IStatus() = default;
		IStatus(const IStatus&) = default;
		IStatus(IStatus&& other) = default;
		virtual ~IStatus() = default;

	public:
		IStatus& operator=(const IStatus&) = default;
		IStatus& operator=(IStatus&& other) = default;

	public:
		virtual std::string_view Name() const = 0;
	};

}
// namespace AqualinkAutomate::Interfaces
