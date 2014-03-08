//  grass - Generic Rail And Signalling Simulator
//
//  WEBSITE: http://bitbucket.org/JGRennison/grass
//  WEBSITE: http://grss.sourceforge.net
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version. See: COPYING-GPL.txt
//
//  This program  is distributed in the  hope that it will  be useful, but
//  WITHOUT   ANY  WARRANTY;   without  even   the  implied   warranty  of
//  MERCHANTABILITY  or FITNESS  FOR A  PARTICULAR PURPOSE.   See  the GNU
//  General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program. If not, see <http://www.gnu.org/licenses/>.
//
//  2013 - Jonathan Rennison <j.g.rennison@gmail.com>
//==========================================================================

#include "common.h"
#include "core/action.h"

void action::Execute() const {
	ExecuteAction();
}

void action::ActionSendReplyFuture(const std::shared_ptr<future> &f) const {
	w.futures.RegisterFuture(f);
}

void action::ActionRegisterFuture(const std::shared_ptr<future> &f) const {
	w.futures.RegisterFuture(f);
}

void action::ActionRegisterLocalFuture(const std::shared_ptr<future> &f) const {
	w.futures.RegisterFuture(f);
}

void action::ActionCancelFuture(future &f) const {
	w.futures.RemoveFuture(f);
}

void action::ActionRegisterFutureAction(futurable_obj &targ, world_time ft, std::unique_ptr<action> &&a) const {
	ActionRegisterLocalFuture(std::make_shared<future_action_wrapper>(targ, ft, w.MakeNewFutureID(), std::move(a)));
}

void future_action_wrapper::ExecuteAction() {
	if(act) act->Execute();
}

void future_action_wrapper::Deserialise(const deserialiser_input &di, error_collection &ec) {
	//TODO: fill this in
}

void future_action_wrapper::Serialise(serialiser_output &so, error_collection &ec) const {
	//TODO: fill this in
}
