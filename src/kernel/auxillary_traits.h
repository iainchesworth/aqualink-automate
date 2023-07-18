#pragma once

#include <any>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Kernel
{

    template<typename TRAIT_TYPE>
    class TraitType
    {
    public:
        using TraitKey = std::string;
        using TraitValue = TRAIT_TYPE;

    public:
        virtual TraitKey Name() const = 0;
        virtual bool IsMutable() const = 0;
    };

    template<typename TRAIT_TYPE>
    class ImmutableTraitType : public TraitType<TRAIT_TYPE>
    {
    public:
        virtual bool IsMutable() const final
        {
            return false;
        }
    };

    template<typename TRAIT_TYPE>
    class MutableTraitType : public TraitType<TRAIT_TYPE>
    {
    public:
        virtual bool IsMutable() const final
        {
            return true;
        }
    };
    
    // Forward declaration
    class Traits;

    template<typename TRAIT_TYPE>
    class TraitValueProxy
    {
    public:
        TraitValueProxy(Traits& traits, TRAIT_TYPE trait_type) :
            m_TraitsRef(traits),
            m_TraitType(trait_type)
        {
        }

    public:
        TraitValueProxy& operator=(TRAIT_TYPE::TraitValue trait_value)
        {
            m_TraitsRef.Set(m_TraitType, trait_value);
            return *this;
        }

    public:
        operator typename TRAIT_TYPE::TraitValue()
        {
            auto value_opt = m_TraitsRef.TryGet(m_TraitType);
            if (!value_opt.has_value())
            {
                throw std::runtime_error("TRAIT VALUE DOES NOT EXIST");
            }

            return value_opt.value();
        }

    private:
        Traits& m_TraitsRef;
        TRAIT_TYPE m_TraitType;
    };

    class Traits
    {
    public:
        template<typename TRAIT_TYPE>
        std::optional<typename TRAIT_TYPE::TraitValue> TryGet(TRAIT_TYPE trait_type)
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
        TraitValueProxy<TRAIT_TYPE> operator[](TRAIT_TYPE trait_type)
        {
            if (!m_Traits.contains(trait_type.Name()))
            {
                throw std::runtime_error("TRAIT DOES NOT EXIST");
            }

            return TraitValueProxy<TRAIT_TYPE>(*this, trait_type);
        }

    public:
        template<typename TRAIT_TYPE>
        bool Has(TRAIT_TYPE trait_type)
        {
            return m_Traits.contains(trait_type.Name());
        }

    public:
        template<typename TRAIT_TYPE>
        void Remove(TRAIT_TYPE trait_type)
        {
            if (m_Traits.contains(trait_type.Name()))
            {
                m_Traits.erase(trait_type.Name());
            }
        }

    public:
        template<typename TRAIT_TYPE>
        bool Set(TRAIT_TYPE trait_type, TRAIT_TYPE::TraitValue trait_value)
        {
            if (!m_Traits.contains(trait_type.Name()))
            {
                // Permitted -> Adding the trait for the first time
                auto [_, was_inserted] = m_Traits.emplace(trait_type.Name(), std::make_any<typename TRAIT_TYPE::TraitValue>(trait_value));
                return was_inserted;
            }
            else if (trait_type.IsMutable())
            {
                // Permitted -> The trait is mutable so the value can be changed.
                m_Traits.at(trait_type.Name()) = std::make_any<typename TRAIT_TYPE::TraitValue>(trait_value);
                return true;
            }
            else
            {
                // NOT Permitted -> immutable trait (which exists) so the value is fixed.
                return false;
            }
        }

    private:
        std::unordered_map<std::string, std::any> m_Traits{};
    };

}
// namespace AqualinkAutomate::Kernel
