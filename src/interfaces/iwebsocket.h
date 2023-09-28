#pragma once

#include <execution>
#include <memory>
#include <string>
#include <unordered_set>

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

	public:
		void HandleOpen(std::shared_ptr<Interfaces::ISession> session, HTTP::Request& req)
		{
			if (nullptr == session)
			{
				///FIXME
			}
			else
			{
                m_ActiveSessions.emplace(session);
                OnOpen(req);
			}
        }

        void HandleMessage(std::shared_ptr<Interfaces::ISession> session, HTTP::Request& req)
        {
            if (nullptr == session)
            {
                /// FIXME
            }
            else
            {
                OnMessage(req);
            }
        }

        void HandleClose(std::shared_ptr<Interfaces::ISession> session, HTTP::Request& req)
        {
            if (nullptr == session)
            {
                /// FIXME
            }
            else
            {
				if (auto session_it = m_ActiveSessions.find(session); m_ActiveSessions.end() != session_it)
				{
                    m_ActiveSessions.erase(session_it);
				}

                OnClose(req);
            }
        }

        void HandleError(std::shared_ptr<Interfaces::ISession> session, HTTP::Request& req)
        {
            if (nullptr == session)
            {
                /// FIXME
            }
            else
            {
                if (auto session_it = m_ActiveSessions.find(session); m_ActiveSessions.end() != session_it)
                {
                    m_ActiveSessions.erase(session_it);
                }

                OnError(req);
            }
        }

	protected:
        virtual void OnOpen(HTTP::Request& req) = 0;
        virtual void OnMessage(HTTP::Request& req) = 0;
        virtual void OnClose(HTTP::Request& req) = 0;
        virtual void OnError(HTTP::Request& req) = 0;

	protected:
        void PublishMessage_AsBinary(std::shared_ptr<Interfaces::ISession> session, const std::string& message)
		{
            if (nullptr == session)
			{
				LogTrace(Channel::Web, "Attempted to publish a binary message to an invalid session (nullptr)");
			}
			else
			{
				// conn->SendBinary(message);
			}
		}

		void PublishMessage_AsText(std::shared_ptr<Interfaces::ISession> session, const std::string& message)
		{
            if (nullptr == session)
			{
				LogTrace(Channel::Web, "Attempted to publish a text message to an invalid session (nullptr)");
			}
			else
			{
				// conn->SendText(message);
			}
		}

	protected:
		void BroadcastMessage_AsBinary(const std::string& message)
		{
            std::for_each(std::execution::par, m_ActiveSessions.cbegin(), m_ActiveSessions.cend(),
				[this, &message](const auto& session) -> void
				{
					PublishMessage_AsBinary(session, message);
				}
			);
		}

		void BroadcastMessage_AsText(const std::string& message)
		{
            std::for_each(std::execution::par, m_ActiveSessions.cbegin(), m_ActiveSessions.cend(),
				[this, &message](const auto& session) -> void
				{
					PublishMessage_AsText(session, message);
				}
			);
		}

	private:
        std::unordered_set<std::shared_ptr<Interfaces::ISession>> m_ActiveSessions;
	};
}
// namespace AqualinkAutomate::Interfaces
