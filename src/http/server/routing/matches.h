#pragma once 

#include <cstddef>
#include <string_view>

namespace AqualinkAutomate::HTTP::Routing
{

    class matches_base
    {
    public:
        using iterator = std::string_view*;
        using const_iterator = std::string_view const*;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference = std::string_view&;
        using const_reference = std::string_view const&;
        using pointer = std::string_view*;
        using const_pointer = std::string_view const*;

        matches_base() = default;

        virtual ~matches_base() = default;

        virtual std::string_view const* matches() const = 0;

        virtual std::string_view const* ids() const = 0;

        virtual std::string_view* matches() = 0;

        virtual std::string_view* ids() = 0;

        virtual std::size_t size() const = 0;

        virtual void resize(std::size_t) = 0;

        const_reference at(size_type pos) const;

        const_reference at(std::string_view id) const;

        const_reference operator[](size_type pos) const;

        const_reference operator[](std::string_view id) const;

        const_iterator find(std::string_view id) const;

        const_iterator begin() const;

        const_iterator end() const;

        bool empty() const noexcept;
    };


    template <std::size_t N = 20>
    class matches_storage : public matches_base
    {
        std::string_view matches_storage_[N];
        std::string_view ids_storage_[N];
        std::size_t size_;

        matches_storage(std::string_view matches[N], std::string_view ids[N], std::size_t n)
        {
            for (std::size_t i = 0; i < n; ++i)
            {
                matches_storage_[i] = matches[i];
                ids_storage_[i] = ids[i];
            }
        }

    public:
        matches_storage() = default;

        virtual std::string_view const* matches() const override
        {
            return matches_storage_;
        }

        virtual std::string_view const* ids() const override
        {
            return ids_storage_;
        }

        virtual std::string_view* matches() override
        {
            return matches_storage_;
        }

        virtual std::string_view* ids() override
        {
            return ids_storage_;
        }

        virtual std::size_t size() const override
        {
            return size_;
        }

        virtual void resize(std::size_t n) override
        {
            size_ = n;
        }
    };

    using matches = matches_storage<20>;

}
// namespace AqualinkAutomate::HTTP::Routing
