#pragma once

#include <algorithm>
#include <memory>
#include <unordered_set>

#include "exceptions/exception_hubnotfound.h"
#include "interfaces/ihub.h"

namespace AqualinkAutomate::Kernel
{

	namespace Hub
	{
		class Wrapper
		{
		public:
			virtual ~Wrapper() = default;
		};

		template <typename T>
		class WrapperImpl : public Wrapper 
		{
		public:
			WrapperImpl(std::shared_ptr<T> h) : 
				m_Hub(h)
			{
			}

		public:
			std::shared_ptr<T> m_Hub;
		};
	}
	// namespace Hub

	class HubLocator
	{
	public:
		template <typename T>
		HubLocator& Register(std::shared_ptr<T> hub)
		{
			m_Hubs.insert(std::make_shared<Hub::WrapperImpl<T>>(hub));
			return *this;
		}

	public:
		template <typename T>
		std::shared_ptr<T> Find() 
		{
			for (const auto& wrapper : m_Hubs) 
			{
				if (auto derived_wrapper = std::dynamic_pointer_cast<Hub::WrapperImpl<T>>(wrapper); derived_wrapper)
				{
					return derived_wrapper->m_Hub;
				}
			}

			throw Exceptions::Hub_NotFound();
		}

	public:
		template <typename T>
		std::shared_ptr<T> TryFind()
		{
			for (const auto& wrapper : m_Hubs)
			{
				if (auto derived_wrapper = std::dynamic_pointer_cast<Hub::WrapperImpl<T>>(wrapper); derived_wrapper)
				{
					return derived_wrapper->m_Hub;
				}
			}

			return nullptr;
		}

	public:
		template <typename T>
		HubLocator& Unregister()
		{
			auto it = std::remove_if(m_Hubs.begin(), m_Hubs.end(), [](const std::shared_ptr<Hub::Wrapper>& wrapper)
				{
					return std::dynamic_pointer_cast<Hub::WrapperImpl<T>>(wrapper) != nullptr;
				});

			m_Hubs.erase(it, m_Hubs.end());

			return *this;
		}

	public:
		std::unordered_set<std::shared_ptr<Hub::Wrapper>> m_Hubs;
	};

}
// namespace AqualinkAutomate::Kernel
