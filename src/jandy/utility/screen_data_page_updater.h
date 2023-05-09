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

#include "logging/logging.h"
#include "jandy/utility/screen_data_page_updater_context.h"
#include "jandy/utility/screen_data_page_updater_evshift.h"
#include "jandy/utility/screen_data_page_updater_evupdate.h"

using namespace AqualinkAutomate::Logging;

namespace AqualinkAutomate::Utility
{

	namespace ScreenDataPageUpdaterImpl
	{		
		class evSequenceStart : public boost::statechart::event<evSequenceStart> {};

		class evSequenceEnd : public boost::statechart::event<evSequenceEnd> {};

		class evClear : public boost::statechart::event<evClear> {};
		
		template<typename PAGE_TYPE> struct Tracking;

		template<typename PAGE_TYPE>
		struct StateMachine : boost::statechart::state_machine<StateMachine<PAGE_TYPE>, Tracking<PAGE_TYPE>>, Context<PAGE_TYPE>
		{
			StateMachine(PAGE_TYPE& page) :
				Context<PAGE_TYPE>(page)
			{
			}
		};

		template<typename PAGE_TYPE>
		struct Tracking : boost::statechart::simple_state<Tracking<PAGE_TYPE>, StateMachine<PAGE_TYPE>>
		{
			typedef boost::mpl::list<
				boost::statechart::custom_reaction<evSequenceStart>,
				boost::statechart::custom_reaction<evSequenceEnd>,
				boost::statechart::custom_reaction<evClear>,
				boost::statechart::custom_reaction<evShift>,
				boost::statechart::custom_reaction<evUpdate>
			> reactions;

			//-----------------------------------------------------------------
			//
			//  Event Handlers
			//
			//-----------------------------------------------------------------

			boost::statechart::result react(const evSequenceStart& ev)
			{
				return this->transit<Tracking<PAGE_TYPE>>();
			}

			boost::statechart::result react(const evSequenceEnd& ev)
			{
				return this->transit<Tracking<PAGE_TYPE>>();
			}

			boost::statechart::result react(const evClear& ev)
			{
				auto& ctx = this->context<StateMachine<PAGE_TYPE>>();

				ctx().Clear();

				return this->transit<Tracking<PAGE_TYPE>>();
			}

			boost::statechart::result react(const evShift& ev)
			{
				auto& ctx = this->context<StateMachine<PAGE_TYPE>>();

				ctx().ShiftLines(ev.Direction(), ev.FirstLineId(), ev.LastLineId(), ev.NumberOfShifts());

				return this->transit<Tracking<PAGE_TYPE>>();
			}

			boost::statechart::result react(const evUpdate& ev)
			{
				auto& ctx = this->context<StateMachine<PAGE_TYPE>>();

				///FIXME Validate the line id is within the size permitted.
				if (ev.Id() >= ctx().Size())
				{
					LogDebug(Channel::Devices, "Attempted to update a page line that is does not exist in the page.");
				}
				else
				{
					ctx()[ev.Id()] = ev.Text();
				}

				return this->transit<Tracking<PAGE_TYPE>>();
			}

		};

	}
	// namespace ScreenDataPageUpdaterImpl

	template<typename PAGE_TYPE>
	using ScreenDataPageUpdater = ScreenDataPageUpdaterImpl::StateMachine<PAGE_TYPE>;

}
// namespace AqualinkAutomate::Utility
