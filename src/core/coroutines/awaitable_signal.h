#pragma once

#include <coroutine>
#include <cstdint>
#include <memory>
#include <optional>

#include <boost/cobalt.hpp>
#include <boost/signals2.hpp>
#include <boost/callable_traits/args.hpp>

#include "developer/async_operation_tracker.h"
#include "developer/signal_connection_tracker.h"

namespace AqualinkAutomate::Coroutines
{

    template<typename SIGNAL>
    struct AwaitableSignal
    {
        using args_type = boost::callable_traits::args_t<typename SIGNAL::signature_type>;

        struct SharedState
        {
            std::coroutine_handle<void> awaited_from;
            std::optional<args_type> result_cache;
        };

        ~AwaitableSignal()
        {
            m_Connection.disconnect();

            // Clear the non-owning handle so a stale slot cleanup
            // never tries to resume a destroyed coroutine.
            if (m_State)
            {
                m_State->awaited_from = nullptr;
            }

            Developer::SignalConnectionTracker::Instance().OnDisconnected(m_DebugConnectionId);
            Developer::AsyncOperationTracker::Instance().OnCompleted(m_DebugAsyncOpId);
        }

        bool await_ready() { return false; }

        void await_suspend(std::coroutine_handle<void> h)
        {
            m_State->awaited_from = h;

            m_DebugConnectionId = Developer::SignalConnectionTracker::Instance().OnConnected("AwaitableSignal");
            m_DebugAsyncOpId = Developer::AsyncOperationTracker::Instance().OnStarted(
                Developer::AsyncOperationType::SignalAwait, "AwaitableSignal::await_suspend");

            m_Connection = signal.connect_extended(
                [state = m_State](const boost::signals2::connection& conn, auto ... args) mutable
                {
                    auto aw = std::exchange(state->awaited_from, nullptr);
                    conn.disconnect();
                    state->result_cache.emplace(std::move(args)...);
                    state.reset();                                      // release shared state ref BEFORE resume
                    aw.resume();                                        // resume the coroutine
                }
            );
        }

        auto await_resume()
        {
            constexpr std::size_t size = std::tuple_size_v<args_type>;

            if constexpr (1u == size)
            {
                return std::get<0u>(*std::move(m_State->result_cache));
            }
            else if constexpr (size > 1u)
            {
                return *std::move(m_State->result_cache);
            }
            else
            {
                return;
            }
        }

        SIGNAL& signal;
        std::shared_ptr<SharedState> m_State = std::make_shared<SharedState>();
        boost::signals2::connection m_Connection;
        uint64_t m_DebugConnectionId{0};
        uint64_t m_DebugAsyncOpId{0};
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
