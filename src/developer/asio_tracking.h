#pragma once 

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <source_location>

#include <boost/asio/execution_context.hpp>
#include <boost/system/error_code.hpp>

#define BOOST_ASIO_INHERIT_TRACKED_HANDLER \
  : public AqualinkAutomate::Developer::AsioTracking::tracked_handler

#define BOOST_ASIO_ALSO_INHERIT_TRACKED_HANDLER \
  , public AqualinkAutomate::Developer::AsioTracking::tracked_handler

#define BOOST_ASIO_HANDLER_TRACKING_INIT \
  AqualinkAutomate::Developer::AsioTracking::init()

#define BOOST_ASIO_HANDLER_LOCATION(args) \
  AqualinkAutomate::Developer::AsioTracking::location args

#define BOOST_ASIO_HANDLER_CREATION(args) \
  AqualinkAutomate::Developer::AsioTracking::creation args

#define BOOST_ASIO_HANDLER_COMPLETION(args) \
  AqualinkAutomate::Developer::AsioTracking::completion tracked_completion args

#define BOOST_ASIO_HANDLER_INVOCATION_BEGIN(args) \
  tracked_completion.invocation_begin args

#define BOOST_ASIO_HANDLER_INVOCATION_END \
  tracked_completion.invocation_end()

#define BOOST_ASIO_HANDLER_OPERATION(args) \
  AqualinkAutomate::Developer::AsioTracking::operation args

#define BOOST_ASIO_HANDLER_REACTOR_REGISTRATION(args) \
  AqualinkAutomate::Developer::AsioTracking::reactor_registration args

#define BOOST_ASIO_HANDLER_REACTOR_DEREGISTRATION(args) \
  AqualinkAutomate::Developer::AsioTracking::reactor_deregistration args

#define BOOST_ASIO_HANDLER_REACTOR_READ_EVENT 1
#define BOOST_ASIO_HANDLER_REACTOR_WRITE_EVENT 2
#define BOOST_ASIO_HANDLER_REACTOR_ERROR_EVENT 4

#define BOOST_ASIO_HANDLER_REACTOR_EVENTS(args) \
  AqualinkAutomate::Developer::AsioTracking::reactor_events args

#define BOOST_ASIO_HANDLER_REACTOR_OPERATION(args) \
  AqualinkAutomate::Developer::AsioTracking::reactor_operation args

namespace AqualinkAutomate::Developer
{

    struct AsioTracking
    {
        // Base class for objects containing tracked handlers.
        struct tracked_handler
        {
            std::uintmax_t handler_id_ = 0; // To uniquely identify a handler.
            std::uintmax_t tree_id_ = 0; // To identify related handlers.
            const char* object_type_; // The object type associated with the handler.
            std::uintmax_t native_handle_; // Native handle, if any.
        };

        // Initialise the tracking system.
        static void init();

        // Record a source location.
        static void location(const char* file_name, int line, const char* function_name);
        static void location(const std::source_location location);

        // Record the creation of a tracked handler.
        static void creation(boost::asio::execution_context& /*ctx*/, tracked_handler& h, const char* object_type, void* /*object*/, std::uintmax_t native_handle, const char* op_name);

        struct completion
        {
            explicit completion(const tracked_handler& h);

            completion(const completion&) = delete;
            completion& operator=(const completion&) = delete;

            // Destructor records only when an exception is thrown from the handler, or
            // if the memory is being freed without the handler having been invoked.
            ~completion()
            {
                *current_completion() = next_;
            }

            // Records that handler is to be invoked with the specified arguments.
            template <class... Args>
            void invocation_begin(Args&&... /*args*/)
            {
                invocation_begin_impl();
            }

            // Records that handler is to be invoked with no arguments.
            void invocation_begin_impl();

            // Record that handler invocation has ended.
            void invocation_end();

            tracked_handler handler_;

            // Completions may nest. Here we stash a pointer to the outer completion.
            completion* next_;
        };

        static completion** current_completion();

        // Record an operation that is not directly associated with a handler.
        static void operation(boost::asio::execution_context& /*ctx*/, const char* /*object_type*/, void* /*object*/, std::uintmax_t /*native_handle*/, const char* /*op_name*/);

        // Record that a descriptor has been registered with the reactor.
        static void reactor_registration(boost::asio::execution_context& context, uintmax_t native_handle, uintmax_t registration);

        // Record that a descriptor has been deregistered from the reactor.
        static void reactor_deregistration(boost::asio::execution_context& context, uintmax_t native_handle, uintmax_t registration);

        // Record reactor-based readiness events associated with a descriptor.
        static void reactor_events(boost::asio::execution_context& context, uintmax_t registration, unsigned events);

        // Record a reactor-based operation that is associated with a handler.
        static void reactor_operation(const tracked_handler& h, const char* op_name, const boost::system::error_code& ec);

        // Record a reactor-based operation that is associated with a handler.
        static void reactor_operation(const tracked_handler& h, const char* op_name, const boost::system::error_code& ec, std::size_t bytes_transferred);
    };

}
// namespace AqualinkAutomate::Developer

#define HANDLER_LOCATION \
  BOOST_ASIO_HANDLER_LOCATION((std::source_location::current()))
