//  grass - Generic Rail And Signalling Simulator
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
#include "core/world.h"
#include "core/serialisable_impl.h"
#include "core/track_ops.h"

action::action(world &w_)
		: w(w_), action_time(w_.GetGameTime()) { }

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

void action::Deserialise(const deserialiser_input &di, error_collection &ec) {

}

void action::Serialise(serialiser_output &so, error_collection &ec) const {
	SerialiseValueJson(GetTypeSerialisationName(), so, "atype");
}

void action::RegisterAllActionTypes(action_deserialisation_type_factory &factory) {
	MakeActionTypeWrapper<action_pointsaction>(factory);
	MakeActionTypeWrapper<action_points_auto_normalise>(factory);
	MakeActionTypeWrapper<action_reservetrack>(factory);
	MakeActionTypeWrapper<action_reservepath>(factory);
	MakeActionTypeWrapper<action_unreservetrackroute>(factory);
	MakeActionTypeWrapper<action_unreservetrack>(factory);
	MakeActionTypeWrapper<action_approachlockingtimeout>(factory);
}

void future_action_wrapper::ExecuteAction() {
	if (act) {
		act->Execute();
	}
}

void future_action_wrapper::Deserialise(const deserialiser_input &di, error_collection &ec) {
	future::Deserialise(di, ec);
	world_obj *wo = dynamic_cast<world_obj *>(&GetTarget());
	if (!wo) {
		ec.RegisterNewError<error_deserialisation>(di, "future_action_wrapper: target is not a world_obj");
		return;
	}
	deserialiser_input subdi(di.json["action"], "action", "action", di);
	if (subdi.json.IsObject()) {
		di.RegisterProp("action");
		act = wo->GetWorld().DeserialiseAction(subdi, ec);
	}
}

void future_action_wrapper::Serialise(serialiser_output &so, error_collection &ec) const {
	future::Serialise(so, ec);
	if (act) {
		so.json_out.String("action");
		so.json_out.StartObject();
		act->Serialise(so, ec);
		so.json_out.EndObject();
	}
}
