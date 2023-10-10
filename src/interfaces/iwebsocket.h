#pragma once

#include <execution>
#include <memory>
#include <string>
#include <unordered_set>

#include <boost/beast/core/flat_buffer.hpp>

#include "concepts/is_c_array.h"
#include "http/server/server_types.h"
#include "interfaces/isession.h"
#include "logging/logging.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Interfaces
{

    class IWebSocketBase
    {
    public:
        IWebSocketBase() = default;
        virtual ~IWebSocketBase() = default;

    public:
        virtual const std::string_view Route() const = 0;

    public:
        void Handle_OnOpen(std::shared_ptr<Interfaces::ISession> session);
        void Handle_OnMessage(std::shared_ptr<Interfaces::ISession> session, const boost::beast::flat_buffer& buffer);
        void Handle_OnClose(std::shared_ptr<Interfaces::ISession> session);
        void Handle_OnError(std::shared_ptr<Interfaces::ISession> session);

    protected:
        virtual void OnOpen() = 0;
        virtual void OnMessage(const boost::beast::flat_buffer& buffer) = 0;
        virtual void OnClose() = 0;
        virtual void OnError() = 0;

    protected:
        void BroadcastMessage_AsBinary(const std::string& message);
        void BroadcastMessage_AsText(const std::string& message);

    private:
        void PublishMessage_AsBinary(std::shared_ptr<Interfaces::ISession> session, const std::string& message);
        void PublishMessage_AsText(std::shared_ptr<Interfaces::ISession> session, const std::string& message);


    private:
        std::unordered_set<std::shared_ptr<Interfaces::ISession>> m_ActiveSessions;
    };

	template<const auto& ROUTE_URL>
	requires (Concepts::CArray<decltype(ROUTE_URL)>)
    class IWebSocket : public IWebSocketBase
	{
	public:
        IWebSocket() = default;
        virtual ~IWebSocket() = default;
	
	public:
        const std::string_view Route() const
        {
            return ROUTE_URL;
        }
	};
}
// namespace AqualinkAutomate::Interfaces
