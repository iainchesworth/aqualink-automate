#pragma once

#include <any>
#include <format>
#include <optional>
#include <unordered_map>

#include <boost/system/error_code.hpp>
#include <tl/expected.hpp>

#include "exceptions/exception_traits_doesnotexist.h"
#include "exceptions/exception_traits_failedtoset.h"
#include "exceptions/exception_traits_notmutable.h"
#include "kernel/auxillary_traits/auxillary_traits_base.h"
#include "kernel/auxillary_traits/auxillary_traits_proxy.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Kernel
{ 

    class Traits
    {
    public:
        template<typename TRAIT_TYPE>
        TraitValueProxy<Traits, TRAIT_TYPE> Get(const TRAIT_TYPE& trait_type)
        {
            if (!m_Traits.contains(trait_type.Name()))
            {
                throw Exceptions::Traits_DoesNotExist();
            }

            return TraitValueProxy<Traits, TRAIT_TYPE>(*this, m_Traits.at(trait_type.Name()));
        }

        template<typename TRAIT_TYPE>
        const ConstTraitValueProxy<Traits, TRAIT_TYPE> Get(const TRAIT_TYPE& trait_type) const
        {
            if (!m_Traits.contains(trait_type.Name()))
            {
                throw Exceptions::Traits_DoesNotExist();
            }

            return ConstTraitValueProxy<Traits, TRAIT_TYPE>(*this, m_Traits.at(trait_type.Name()));
        }

    public:
        template<typename TRAIT_TYPE>
        TraitValueProxy<Traits, TRAIT_TYPE> operator[](const TRAIT_TYPE& trait_type)
        {
            return Get(trait_type);
        }

        template<typename TRAIT_TYPE>
        const ConstTraitValueProxy<Traits, TRAIT_TYPE> operator[](const TRAIT_TYPE& trait_type) const
        {
            return Get(trait_type);
        }

    public:
        template<typename TRAIT_TYPE>
        bool Has(TRAIT_TYPE trait_type) const
        {
            return m_Traits.contains(trait_type.Name());
        }

    public:
        template<typename TRAIT_TYPE>
        bool Remove(TRAIT_TYPE trait_type)
        {
            if (!m_Traits.contains(trait_type.Name()))
            {
                // IMPOSSIBLE -> Trait does not exist in the collection.
            }
            else if (!trait_type.IsMutable())
            {
                // NOT PERMITTED -> Trait is immutable so cannot be removed
            }
            else
            {
                // PERMITTED -> mutable trait...remove it.
                return (0 < m_Traits.erase(trait_type.Name()));
            }

            return false;
        }

    public:
        template<typename TRAIT_TYPE>
        void Set(const TRAIT_TYPE& trait_type, TRAIT_TYPE::TraitValue trait_value)
        {
            if (!m_Traits.contains(trait_type.Name()))
            {
                // PERMITTED -> Adding the trait for the first time
                auto [_, was_inserted] = m_Traits.emplace(trait_type.Name(), std::make_any<typename TRAIT_TYPE::TraitValue>(trait_value));
                if (!was_inserted)
                {
                    throw Exceptions::Traits_FailedToSet();
                }
            }
            else if (trait_type.IsMutable())
            {
                // PERMITTED -> The trait is mutable so the value can be changed.
                m_Traits.at(trait_type.Name()) = std::make_any<typename TRAIT_TYPE::TraitValue>(trait_value);
            }
            else
            {
                // NOT PERMITTED -> immutable trait (which exists) so the value is fixed.
                throw Exceptions::Traits_NotMutable();
            }
        }

    public:
        template<typename TRAIT_TYPE>
        std::optional<typename TRAIT_TYPE::TraitValue> TryGet(const TRAIT_TYPE& trait_type) const
        {
            if (auto it = m_Traits.find(trait_type.Name()); m_Traits.end() == it)
            {
                LogTrace(Channel::Devices, std::format("Requested device trait ({}) does not exist", trait_type.Name()));
            }
            else if (!(it->second.has_value()))
            {
                LogDebug(Channel::Devices, std::format("Requested device trait ({}) does not have an associated value", trait_type.Name()));
            }
            else
            {
                try
                {
                    return std::any_cast<typename TRAIT_TYPE::TraitValue>(it->second);
                }
                catch (const std::bad_any_cast& exBAC)
                {
                    LogDebug(Channel::Devices, std::format("Requested device trait ({}) associated value is the incorrect type", trait_type.Name()));
                }
            }

            return std::nullopt;
        }

    public:
        template<typename TRAIT_TYPE>
        tl::expected<std::reference_wrapper<Traits>, boost::system::error_code> TrySet(const TRAIT_TYPE& trait_type, TRAIT_TYPE::TraitValue trait_value)
        {
            if (!m_Traits.contains(trait_type.Name()))
            {
                // PERMITTED -> Adding the trait for the first time
                auto [_, was_inserted] = m_Traits.emplace(trait_type.Name(), std::make_any<typename TRAIT_TYPE::TraitValue>(trait_value));
                if (!was_inserted)
                {
                    /// FAILED -> Could not add the trait for some unspecified reason.
                    return tl::unexpected(boost::system::errc::make_error_code(boost::system::errc::operation_not_permitted));
                }
            }
            else if (trait_type.IsMutable())
            {
                // PERMITTED -> The trait is mutable so the value can be changed.
                m_Traits.at(trait_type.Name()) = std::make_any<typename TRAIT_TYPE::TraitValue>(trait_value);
            }
            else
            {
                // NOT PERMITTED -> immutable trait (which exists) so the value is fixed.
                return tl::unexpected(boost::system::errc::make_error_code(boost::system::errc::operation_not_permitted));
            }

            return *this;
        }

    private:
        std::unordered_map<std::string, std::any> m_Traits{};
    };

}
// namespace AqualinkAutomate::Kernel
