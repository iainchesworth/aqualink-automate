#pragma once

#include <boost/circular_buffer.hpp>
#include <boost/circular_buffer/space_optimized.hpp>

namespace AqualinkAutomate::Utility
{
	template<typename T, typename U = boost::circular_buffer_space_optimized<T>>
	class circular_buffer_wrapping_iterator 
	{
	public:
		using value_type = T;
		using reference = value_type&;
		using pointer = value_type*;
		using difference_type = std::ptrdiff_t;
		using iterator_category = std::random_access_iterator_tag;

		circular_buffer_wrapping_iterator(U& buf, std::size_t pos) : buf_(buf), pos_(pos)
		{
		}

		reference operator*() const 
		{
			return buf_[pos_];
		}

		circular_buffer_wrapping_iterator& operator++() 
		{
			if (++pos_ == buf_.size())
				pos_ = 0;
			return *this;
		}

		circular_buffer_wrapping_iterator& operator--() 
		{
			if (pos_-- == 0)
				pos_ = buf_.size() - 1;
			return *this;
		}

		circular_buffer_wrapping_iterator operator++(int) 
		{
			circular_buffer_wrapping_iterator tmp(*this);
			++(*this);
			return tmp;
		}

		circular_buffer_wrapping_iterator operator--(int) 
		{
			circular_buffer_wrapping_iterator tmp(*this);
			--(*this);
			return tmp;
		}

		circular_buffer_wrapping_iterator& operator+=(difference_type n) 
		{
			pos_ = (pos_ + n) % buf_.size();
			return *this;
		}

		circular_buffer_wrapping_iterator& operator-=(difference_type n) 
		{
			pos_ = (pos_ + buf_.size() - n) % buf_.size();
			return *this;
		}

		circular_buffer_wrapping_iterator operator+(difference_type n) const 
		{
			circular_buffer_wrapping_iterator tmp(*this);
			return tmp += n;
		}

		circular_buffer_wrapping_iterator operator-(difference_type n) const 
		{
			circular_buffer_wrapping_iterator tmp(*this);
			return tmp -= n;
		}

		difference_type operator-(const circular_buffer_wrapping_iterator& rhs) const 
		{
			if (buf_ != rhs.buf_)
				throw std::runtime_error("iterators not compatible");
			if (pos_ >= rhs.pos_)
				return pos_ - rhs.pos_;
			else
				return buf_.size() - rhs.pos_ + pos_;
		}

		reference operator[](difference_type n) const 
		{
			return *(*this + n);
		}

		bool operator==(const circular_buffer_wrapping_iterator& rhs) const 
		{
			return (buf_ == rhs.buf_) && (pos_ == rhs.pos_);
		}

		bool operator!=(const circular_buffer_wrapping_iterator& rhs) const 
		{
			return !(*this == rhs);
		}

		bool operator<(const circular_buffer_wrapping_iterator& rhs) const 
		{
			if (buf_ != rhs.buf_)
				throw std::runtime_error("iterators not compatible");
			return (pos_ < rhs.pos_) || (pos_ > rhs.pos_ && pos_ < rhs.buf_.size());
		}

		bool operator>(const circular_buffer_wrapping_iterator& rhs) const 
		{
			return rhs < *this;
		}

		bool operator<=(const circular_buffer_wrapping_iterator& rhs) const 
		{
			return !(*this > rhs);
		}

		bool operator>=(const circular_buffer_wrapping_iterator& rhs) const 
		{
			return !(*this < rhs);
		}

		circular_buffer_wrapping_iterator begin() const 
		{
			return circular_buffer_wrapping_iterator(buf_, 0);
		}

		circular_buffer_wrapping_iterator end() const 
		{
			return circular_buffer_wrapping_iterator(buf_, buf_.size());
		}

	private:
		U& buf_;
		std::size_t pos_;
	};
}
// namespace AqualinkAutomate::Utility
