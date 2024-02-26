#include <format>

#include "developer/asio_tracking.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Developer
{

    void AsioTracking::init()
    {
    }

    void AsioTracking::location(const char* file_name, int line, const char* function_name)
    {
        LogFatal(
            Channel::Developer,
            std::format(
                "At location {}:{:d} in {}",
                file_name,
                line,
                function_name
            )
        );
    }

    void AsioTracking::location(const std::source_location location)
    {
        LogFatal(
            Channel::Developer, 
            std::format(
                "At location {}:{:d} in {}", 
                location.file_name(), 
                location.line(), 
                location.function_name()
            )
        );
    }

    void AsioTracking::creation(boost::asio::execution_context& /*ctx*/, tracked_handler& h, const char* object_type, void* /*object*/, std::uintmax_t native_handle, const char* op_name)
    {
        // Generate a unique id for the new handler.
        static std::atomic<std::uintmax_t> next_handler_id{ 1 };
        h.handler_id_ = next_handler_id++;

        // Copy the tree identifier forward from the current handler.
        if (*current_completion())
            h.tree_id_ = (*current_completion())->handler_.tree_id_;

        // Store various attributes of the operation to use in later output.
        h.object_type_ = object_type;
        h.native_handle_ = native_handle;

        LogFatal(
            Channel::Developer, 
            std::format(
                "Starting operation {}.{} for native_handle = {}, handler = {}, tree = {}",
                object_type, 
                op_name, 
                h.native_handle_, 
                h.handler_id_, 
                h.tree_id_
            )
        );
    }

    AsioTracking::completion::completion(const tracked_handler& h) :
        handler_(h),
        next_(*current_completion())
    {
        *current_completion() = this;
    }

    void AsioTracking::completion::invocation_begin_impl()
    {
        LogFatal(Channel::Developer, std::format("Entering handler {} in tree {}", handler_.handler_id_, handler_.tree_id_));
    }

    void AsioTracking::completion::invocation_end()
    {
        LogFatal(Channel::Developer, std::format("Leaving handler {} in tree {}", handler_.handler_id_, handler_.tree_id_));
    }

    AsioTracking::completion** AsioTracking::current_completion()
    {
        static BOOST_ASIO_THREAD_KEYWORD completion* current = nullptr;
        return &current;
    }

    // Record an operation that is not directly associated with a handler.
    void AsioTracking::operation(boost::asio::execution_context& /*ctx*/, const char* /*object_type*/, void* /*object*/, std::uintmax_t /*native_handle*/, const char* /*op_name*/)
    {
    }

    // Record that a descriptor has been registered with the reactor.
    void AsioTracking::reactor_registration(boost::asio::execution_context& context, uintmax_t native_handle, uintmax_t registration)
    {
        LogFatal(Channel::Developer, std::format("Adding to reactor native_handle = {}, registration = {}", native_handle, registration));
    }

    // Record that a descriptor has been deregistered from the reactor.
    void AsioTracking::reactor_deregistration(boost::asio::execution_context& context, uintmax_t native_handle, uintmax_t registration)
    {
        LogFatal(Channel::Developer, std::format("Removing from reactor native_handle = {}, registration = {}", native_handle, registration));
    }

    // Record reactor-based readiness events associated with a descriptor.
    void AsioTracking::reactor_events(boost::asio::execution_context& context, uintmax_t registration, unsigned events)
    {
        LogFatal(
            Channel::Developer,
            std::format(
                "Reactor readiness for registration = {}, events = {}{}{}",
                registration,
                (events & BOOST_ASIO_HANDLER_REACTOR_READ_EVENT) ? " read" : "",
                (events & BOOST_ASIO_HANDLER_REACTOR_WRITE_EVENT) ? " write" : "",
                (events & BOOST_ASIO_HANDLER_REACTOR_ERROR_EVENT) ? " error" : ""
            )
        );
    }

    // Record a reactor-based operation that is associated with a handler.
    void AsioTracking::reactor_operation(const tracked_handler& h, const char* op_name, const boost::system::error_code& ec)
    {
        LogFatal(
            Channel::Developer,
            std::format(
                "Performed operation {}.{} for native_handle = {}, ec = {}:{}",
                h.object_type_,
                op_name,
                h.native_handle_,
                ec.category().name(),
                ec.value()
            )
        );
    }

    // Record a reactor-based operation that is associated with a handler.
    void AsioTracking::reactor_operation(const tracked_handler& h, const char* op_name, const boost::system::error_code& ec, std::size_t bytes_transferred)
    {
        LogFatal(
            Channel::Developer,
            std::format(
                "Performed operation %s.%s for native_handle = {}, ec = {}:{}, n = {}",
                h.object_type_,
                op_name,
                h.native_handle_,
                ec.category().name(),
                ec.value(),
                static_cast<uintmax_t>(bytes_transferred)
            )
        );
    }

}
// namespace AqualinkAutomate::Developer
