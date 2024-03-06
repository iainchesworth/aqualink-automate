#pragma once

#include <coroutine>
#include <optional>

#include <boost/cobalt.hpp>
#include <boost/signals2.hpp>
#include <boost/callable_traits/args.hpp>

namespace AqualinkAutomate::Coroutines
{

    template<typename SIGNAL>
    struct AwaitableSignal
    {
        using args_type = boost::callable_traits::args_t<typename SIGNAL::signature_type>;

        bool await_ready() { return false; }

        void await_suspend(std::coroutine_handle<void> h)
        {
            awaited_from.reset(h.address());

            // The handler will get copied, so we can't capture the handle with a unique_ptr
            signal.connect_extended(
                [this, _ = boost::intrusive_ptr<AwaitableSignal>(this)](const boost::signals2::connection& conn, auto ... args) mutable
                {
                    auto aw = std::move(awaited_from);
                    conn.disconnect();
                    result_cache.emplace(std::move(args)...);   // the result_catch lives in the coro frame
                    std::move(aw).resume();                     // release ownership & resume
                }
            );
        }

        auto await_resume() // return the value
        {
            constexpr std::size_t size = std::tuple_size_v<args_type>;

            if constexpr (1u == size) // single argument doesn't need a tuple
            {
                return std::get<0u>(*std::move(result_cache));
            }
            else if constexpr (size > 1u) // make a tuple if more than one arg
            {
                return *std::move(result_cache);
            }
            else
            {
                return; // void
            }
        }

        SIGNAL& signal;                                    // capture it for lazy initialization
        boost::cobalt::unique_handle<void> awaited_from;   // capture the ownership of the awaiting coroutine
        std::optional<args_type> result_cache;             // store the result from the call

        // to manage shared ownership with an internal counter.
        // If the last gets released before the handler is invoked,
        // the coro just gets destroyed.

        std::size_t use_count{ 0u };

        friend void intrusive_ptr_add_ref(AwaitableSignal* st)
        {
            st->use_count++;
        }

        friend void intrusive_ptr_release(AwaitableSignal* st)
        {
            if (st->use_count-- == 1u)
            {
                st->awaited_from.reset();
            }
        }
    };

    template<typename SIGNAL_PAYLOAD>
    static auto AwaitSignal(boost::signals2::signal<void(SIGNAL_PAYLOAD)>& sig) -> boost::cobalt::promise<SIGNAL_PAYLOAD>
    {
        co_return co_await sig;
    }

}
// namespace AqualinkAutomate::Coroutines

namespace boost::signals2
{

    template<typename ... ARGS>
    auto operator co_await(boost::signals2::signal<ARGS...>& sig) -> AqualinkAutomate::Coroutines::AwaitableSignal<boost::signals2::signal<ARGS...>>
    {
        return { sig };
    }

}
// namespace boost::signals2
