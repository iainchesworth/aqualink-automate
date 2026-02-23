#include <stdexcept>

#include <boost/assert.hpp>
#include <boost/throw_exception.hpp>

#include "http/server/routing/matches.h"

namespace AqualinkAutomate::HTTP::Routing
{

    matches_base::const_reference matches_base::at(size_type pos) const
    {
        if (pos < size())
        {
            return matches()[pos];
        }

        boost::throw_exception(std::out_of_range(""));
    }

    matches_base::const_reference matches_base::operator[](size_type pos) const
    {
        BOOST_ASSERT(pos < size());
        return matches()[pos];
    }

    matches_base::const_reference matches_base::at(std::string_view id) const
    {
        for (std::size_t i = 0; i < size(); ++i)
        {
            if (ids()[i] == id)
            {
                return matches()[i];
            }
        }

        boost::throw_exception(std::out_of_range(""));
    }

    matches_base::const_reference matches_base::operator[](std::string_view id) const
    {
        return at(id);
    }

    matches_base::const_iterator matches_base::find(std::string_view id) const
    {
        for (std::size_t i = 0; i < size(); ++i)
        {
            if (ids()[i] == id)
            {
                return matches() + i;
            }
        }

        return matches() + size();
    }

    matches_base::const_iterator matches_base::begin() const
    {
        return &matches()[0];
    }

    matches_base::const_iterator matches_base::end() const
    {
        return &matches()[size()];
    }

    bool matches_base::empty() const noexcept
    {
        return size() == 0;
    }

}
// namespace AqualinkAutomate::HTTP::Routing
