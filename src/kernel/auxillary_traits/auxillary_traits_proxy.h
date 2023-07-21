#pragma once

#include <any>
#include <format>

#include "exceptions/exception_traits_doesnotexist.h"
#include "exceptions/exception_traits_invalidtraitvalue.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Kernel
{

    template<typename TRAITS, typename TRAIT_TYPE>
    class TraitValueProxy
    {
    public:
        using ValueType = typename TRAIT_TYPE::TraitValue;
        using Reference = ValueType&;
        using Pointer = ValueType*;

    public:
        TraitValueProxy(TRAITS& traits, std::any& trait_valueany) :
            m_TraitsRef(traits),
            m_ValueAny(trait_valueany)
        {
        }

    public:
        TraitValueProxy& operator=(TRAIT_TYPE::TraitValue trait_value)
        {
            m_TraitsRef.Set(TRAIT_TYPE{}, trait_value);
            return *this;
        }

    public:
        operator auto() const
        {
            try
            {
                return std::any_cast<ValueType>(m_ValueAny);
            }
            catch (const std::bad_any_cast& exBAC)
            {
                LogDebug(Channel::Devices, std::format("Attempted to retrieve invalid trait value: type -> {}", TRAIT_TYPE{}.Name()));
                throw Exceptions::Traits_InvalidTraitValue();
            }
            catch (const std::out_of_range& exOOR)
            {
                LogDebug(Channel::Devices, std::format("Attempted to retrieve non-existent trait: type -> {}", TRAIT_TYPE{}.Name()));
                throw Exceptions::Traits_DoesNotExist();
            }
        }

    public:
        Reference operator*()
        {
            try
            {
                return std::any_cast<Reference>(m_ValueAny);
            }
            catch (const std::bad_any_cast& exBAC)
            {
                LogDebug(Channel::Devices, std::format("Attempted to retrieve invalid trait value: type -> {}", TRAIT_TYPE{}.Name()));
                throw Exceptions::Traits_InvalidTraitValue();
            }
            catch (const std::out_of_range& exOOR)
            {
                LogDebug(Channel::Devices, std::format("Attempted to retrieve non-existent trait: type -> {}", TRAIT_TYPE{}.Name()));
                throw Exceptions::Traits_DoesNotExist();
            }
        }

        Pointer operator->()
        {
            try
            {
                return std::any_cast<Pointer>(m_ValueAny);
            }
            catch (const std::bad_any_cast& exBAC)
            {
                LogDebug(Channel::Devices, std::format("Attempted to retrieve invalid trait value: type -> {}", TRAIT_TYPE{}.Name()));
                throw Exceptions::Traits_InvalidTraitValue();
            }
            catch (const std::out_of_range& exOOR)
            {
                LogDebug(Channel::Devices, std::format("Attempted to retrieve non-existent trait: type -> {}", TRAIT_TYPE{}.Name()));
                throw Exceptions::Traits_DoesNotExist();
            }
        }

    private:
        TRAITS& m_TraitsRef;
        std::any& m_ValueAny;
    };

    template<typename TRAITS, typename TRAIT_TYPE>
    class ConstTraitValueProxy
    {
    public:
        using ValueType = TRAIT_TYPE::TraitValue;
        using Reference = const ValueType&;
        using Pointer = const ValueType*;

    public:
        ConstTraitValueProxy(const TRAITS& traits, const std::any& trait_valueany) :
            m_TraitsRef(traits),
            m_ValueAny(trait_valueany)
        {
        }

    public:
        operator auto() const
        {
            try
            {
                return std::any_cast<ValueType>(m_ValueAny);
            }
            catch (const std::bad_any_cast& exBAC)
            {
                LogDebug(Channel::Devices, std::format("Attempted to retrieve invalid trait value: type -> {}", TRAIT_TYPE{}.Name()));
                throw Exceptions::Traits_InvalidTraitValue();
            }
            catch (const std::out_of_range& exOOR)
            {
                LogDebug(Channel::Devices, std::format("Attempted to retrieve non-existent trait: type -> {}", TRAIT_TYPE{}.Name()));
                throw Exceptions::Traits_DoesNotExist();
            }
        }

    public:
        Reference operator*() const
        {
            try
            {
                return std::any_cast<Reference>(m_ValueAny);
            }
            catch (const std::bad_any_cast& exBAC)
            {
                LogDebug(Channel::Devices, std::format("Attempted to retrieve invalid trait value: type -> {}", TRAIT_TYPE{}.Name()));
                throw Exceptions::Traits_InvalidTraitValue();
            }
            catch (const std::out_of_range& exOOR)
            {
                LogDebug(Channel::Devices, std::format("Attempted to retrieve non-existent trait: type -> {}", TRAIT_TYPE{}.Name()));
                throw Exceptions::Traits_DoesNotExist();
            }
        }
        
        Pointer operator->() const
        {
            return &(operator*());
        }

    private:
        const TRAITS& m_TraitsRef;
        const std::any& m_ValueAny;
    };

}
// namespace AqualinkAutomate::Kernel
