#pragma once

#include <cstdint>
#include <string>
#include <utility>

#include <boost/mpl/list.hpp>
#include <boost/statechart/event.hpp>
#include <boost/statechart/custom_reaction.hpp>
#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/transition.hpp>

namespace AqualinkAutomate::Utility
{

	namespace ScreenDataPageUpdaterImpl
	{

		template<typename PAGE_TYPE>
		class Context
		{
		public:
			Context(PAGE_TYPE& page) :
				Page(page)
			{
			};

		public:
			PAGE_TYPE& Page;
		};

		class evStart : public boost::statechart::event<evStart> {};
		class evEnd : public boost::statechart::event<evEnd> {};

		class evUpdating : public boost::statechart::event<evUpdating>
		{
		public:
			using Line = std::pair<uint8_t, std::string>;

		public:
			evUpdating(uint8_t line_id, const std::string& line_text) :
				m_Line({ line_id, line_text })
			{
			}

			evUpdating(Line& line) :
				m_Line(line)
			{
			}

			Line::first_type Id() const
			{
				return m_Line.first;
			}

			Line::second_type Text() const
			{
				return m_Line.second;
			}

		private:
			const Line m_Line;
		};

		template<typename PAGE_TYPE> struct Waiting;
		template<typename PAGE_TYPE> struct Updating;

		template<typename PAGE_TYPE>
		struct StateMachine : boost::statechart::state_machine<StateMachine<PAGE_TYPE>, Waiting<PAGE_TYPE>>, Context<PAGE_TYPE>
		{
			StateMachine(PAGE_TYPE& page) :
				Context<PAGE_TYPE>(page)
			{
			}
		};

		template<typename PAGE_TYPE>
		struct Waiting : boost::statechart::simple_state<Waiting<PAGE_TYPE>, StateMachine<PAGE_TYPE>>
		{
			typedef boost::statechart::custom_reaction<evStart> reactions;

			boost::statechart::result react(const evStart& ev)
			{
				auto& ctx = this->context<StateMachine<PAGE_TYPE>>();

				for (auto& line : ctx.Page)
				{
					line.clear();
				}

				return this->transit< Updating<PAGE_TYPE>>();
			}
		};

		template<typename PAGE_TYPE>
		struct Updating : boost::statechart::simple_state<Updating<PAGE_TYPE>, StateMachine<PAGE_TYPE>>
		{
			typedef boost::mpl::list<
				boost::statechart::custom_reaction<evUpdating>,
				boost::statechart::transition<evEnd, Waiting<PAGE_TYPE>>
			> reactions;

			boost::statechart::result react(const evUpdating& ev)
			{
				auto& ctx = this->context<StateMachine<PAGE_TYPE>>();

				ctx.Page[ev.Id()] = ev.Text();

				return this->transit<Updating<PAGE_TYPE>>();
			}
		};

	}
	// namespace ScreenDataPageUpdaterImpl

	template<typename PAGE_TYPE>
	using ScreenDataPageUpdater = ScreenDataPageUpdaterImpl::StateMachine<PAGE_TYPE>;

}
// namespace AqualinkAutomate::Utility
