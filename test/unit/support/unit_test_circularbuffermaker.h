#pragma once

#include <cstdint>
#include <initializer_list>

#include <boost/circular_buffer.hpp>

namespace AqualinkAutomate::Test
{

    template <typename T>
    boost::circular_buffer<T> MakeCircularBuffer(std::initializer_list<T> init)
    {
        boost::circular_buffer<T> buffer(init.size());      // capacity = number of elements
        buffer.assign(init.begin(), init.end());            // copy elements into the buffer
        return buffer;
    }

}
// namespace AqualinkAutomate::Test
