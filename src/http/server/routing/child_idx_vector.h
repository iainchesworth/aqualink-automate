#pragma once

#include <cstddef>
#include <cstring>

#include <boost/assert.hpp>

namespace AqualinkAutomate::HTTP::Routing
{

    class child_idx_vector
    {
        static constexpr std::size_t N = 5;
        std::size_t static_child_idx_[N]{};
        std::size_t* child_idx{ nullptr };
        std::size_t size_{ 0 };
        std::size_t cap_{ 0 };

    public:
        ~child_idx_vector()
        {
            delete[] child_idx;
        }

        child_idx_vector() = default;

        child_idx_vector(child_idx_vector const& other) : 
            size_{ other.size_ }, 
            cap_{ other.cap_ }
        {
            if (other.child_idx)
            {
                child_idx = new std::size_t[cap_];
                std::memcpy(child_idx, other.child_idx, size_ * sizeof(std::size_t));
                return;
            }

            std::memcpy(static_child_idx_, other.static_child_idx_, size_ * sizeof(std::size_t));
        }

        child_idx_vector(child_idx_vector&& other) : 
            child_idx{ other.child_idx }, 
            size_{ other.size_ }, 
            cap_{ other.cap_ }
        {
            std::memcpy(static_child_idx_, other.static_child_idx_, N);
            other.child_idx = nullptr;
        }

        bool empty() const
        {
            return size_ == 0;
        }

        std::size_t size() const
        {
            return size_;
        }

        std::size_t* begin()
        {
            if (child_idx)
            {
                return child_idx;
            }

            return static_child_idx_;
        }

        std::size_t* end()
        {
            return begin() + size_;
        }

        std::size_t const* begin() const
        {
            if (child_idx)
            {
                return child_idx;
            }

            return static_child_idx_;
        }

        std::size_t const* end() const
        {
            return begin() + size_;
        }

        void erase(std::size_t* it)
        {
            BOOST_ASSERT(it - begin() >= 0);
            std::memmove(it - 1, it, end() - it);
            --size_;
        }

        void push_back(std::size_t v)
        {
            if (size_ == N && !child_idx)
            {
                child_idx = new std::size_t[N * 2];
                cap_ = N * 2;
                std::memcpy(child_idx, static_child_idx_, N * sizeof(std::size_t));
            }
            else if (child_idx && size_ == cap_)
            {
                auto* tmp = new std::size_t[cap_ * 2];
                std::memcpy(tmp, child_idx, cap_ * sizeof(std::size_t));
                delete[] child_idx;
                child_idx = tmp;
                cap_ = cap_ * 2;
            }

            begin()[size_++] = v;
        }
    };

}
// namespace AqualinkAutomate::HTTP::Routing
