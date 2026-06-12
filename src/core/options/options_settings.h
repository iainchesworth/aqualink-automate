#pragma once

#include <any>
#include <expected>
#include <format>
#include <functional>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

#include "concepts/is_option_processor.h"

namespace AqualinkAutomate::Options
{

	class Settings
	{
	public:
		template<typename T>
		void Set(const std::string& area, T&& value)
		{
			m_Data[area] = std::forward<T>(value);
		}

		template<Concepts::HasAreaName T>
		[[nodiscard]] std::expected<std::reference_wrapper<const T>, std::string> Get() const
		{
			const std::string& area = T::AreaName();
			
			auto it = m_Data.find(area);
			if (it == m_Data.end())
			{
				return std::unexpected(std::format("Settings area '{}' not found", area));
			}

			const T* ptr = std::any_cast<T>(&it->second);
			if (!ptr)
			{
				return std::unexpected(std::format("Settings area '{}' has wrong type (expected {})", area, typeid(T).name()));
			}

			const T& ref = *ptr;
			return ref;
		}

		[[nodiscard]] bool Has(const std::string& area) const
		{
			return m_Data.contains(area);
		}

		[[nodiscard]] std::vector<std::string> GetAreas() const
		{
			std::vector<std::string> areas;
			areas.reserve(m_Data.size());
			for (const auto& [area, _] : m_Data)
			{
				areas.push_back(area);
			}
			return areas;
		}

		void Clear()
		{
			m_Data.clear();
		}

	private:
		std::unordered_map<std::string, std::any> m_Data;
	};

}
// namespace AqualinkAutomate::Options
