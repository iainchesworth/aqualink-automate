#include "interfaces/iwebsocket.h"

namespace AqualinkAutomate::Interfaces
{
    
    void IWebSocketBase::Handle_OnOpen(std::shared_ptr<Interfaces::ISession> session)
    {
        if (nullptr == session)
        {
            ///FIXME
        }
        else
        {
            m_ActiveSessions.emplace(session);
            OnOpen();
        }
    }

    void IWebSocketBase::Handle_OnMessage(std::shared_ptr<Interfaces::ISession> session, const boost::beast::flat_buffer& buffer)
    {
        if (nullptr == session)
        {
            /// FIXME
        }
        else
        {
            OnMessage(buffer);
        }
    }

    void IWebSocketBase::Handle_OnClose(std::shared_ptr<Interfaces::ISession> session)
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

            OnClose();
        }
    }

    void IWebSocketBase::Handle_OnError(std::shared_ptr<Interfaces::ISession> session)
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

            OnError();
        }
    }

    void IWebSocketBase::BroadcastMessage_AsBinary(const std::string& message)
    {
        std::for_each(std::execution::par, m_ActiveSessions.cbegin(), m_ActiveSessions.cend(),
            [this, &message](const auto& session) -> void
            {
                PublishMessage_AsBinary(session, message);
            }
        );
    }

    void IWebSocketBase::BroadcastMessage_AsText(const std::string& message)
    {
        std::for_each(std::execution::par, m_ActiveSessions.cbegin(), m_ActiveSessions.cend(),
            [this, &message](const auto& session) -> void
            {
                PublishMessage_AsText(session, message);
            }
        );
    }

    void IWebSocketBase::PublishMessage_AsBinary(std::shared_ptr<Interfaces::ISession> session, const std::string& message)
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

    void IWebSocketBase::PublishMessage_AsText(std::shared_ptr<Interfaces::ISession> session, const std::string& message)
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

}
// namespace AqualinkAutomate::Interfaces
