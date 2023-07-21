#pragma once

#include <string>

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

}
// namespace AqualinkAutomate::Kernel
