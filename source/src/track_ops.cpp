//  grass - Generic Rail And Signalling Simulator
//
//  WEBSITE: http://grss.sourceforge.net
//
//  NOTE: This software is licensed under the GPL. See: COPYING-GPL.txt
//
//  This program  is distributed in the  hope that it will  be useful, but
//  WITHOUT   ANY  WARRANTY;   without  even   the  implied   warranty  of
//  MERCHANTABILITY  or FITNESS  FOR A  PARTICULAR PURPOSE.   See  the GNU
//  General Public License for more details.
//
//  Jonathan Rennison (or anybody else) is in no way responsible, or liable
//  for this program or its use in relation to users, 3rd parties or to any
//  persons in any way whatsoever.
//
//  You  should have  received a  copy of  the GNU  General Public
//  License along  with this program; if  not, write to  the Free Software
//  Foundation, Inc.,  59 Temple Place,  Suite 330, Boston,  MA 02111-1307
//  USA
//
//  2013 - Jonathan Rennison <j.g.rennison@gmail.com>
//==========================================================================

#include "common.h"
#include "track_ops.h"
#include "serialisable_impl.h"
#include "signal.h"
#include <memory>

void future_pointsaction::ExecuteAction() {
	genericpoints *gp = dynamic_cast<genericpoints *>(&GetTarget());
	if(gp) {
		gp->SetPointFlagsMasked(index, bits, mask);
	}
}

void future_pointsaction::Deserialise(const deserialiser_input &di, error_collection &ec) {
	future::Deserialise(di, ec);
	CheckTransJsonValueDef(index, di, "index", 0, ec);
	CheckTransJsonValueDef(bits, di, "bits", 0, ec);
	CheckTransJsonValueDef(mask, di, "mask", 0, ec);
}

void future_pointsaction::Serialise(serialiser_output &so, error_collection &ec) const {
	future::Serialise(so, ec);
	SerialiseValueJson(index, so, "index");
	SerialiseValueJson(bits, so, "bits");
	SerialiseValueJson(mask, so, "mask");
}

void action_pointsaction::ExecuteAction() const {
	if(!target) return;

	unsigned int old_pflags = target->GetPointFlags(index);
	unsigned int change_flags = (old_pflags ^ bits) & mask;

	unsigned immediate_action_bits = 0;
	unsigned immediate_action_mask = 0;

	if(change_flags & genericpoints::PTF_REV) {
		if(old_pflags & genericpoints::PTF_LOCKED) {
			ActionSendReplyFuture(std::make_shared<future_pointsactionmessage>(*target, action_time+1, &w, "track_ops/pointsunmovable", "points/locked"));
		}
		else if(old_pflags & genericpoints::PTF_REMINDER) {
			ActionSendReplyFuture(std::make_shared<future_pointsactionmessage>(*target, action_time+1, &w, "track_ops/pointsunmovable", "points/reminderset"));
		}
		else if(target->GetFlags(target->GetDefaultValidDirecton()) & generictrack::GTF_ROUTESET) {
			ActionSendReplyFuture(std::make_shared<future_pointsactionmessage>(*target, action_time+1, &w, "track_ops/pointsunmovable", "track/reserved"));
		}
		else {
			CancelFutures(index, 0, genericpoints::PTF_OOC);
			immediate_action_bits |= (bits & genericpoints::PTF_REV) | genericpoints::PTF_OOC;
			immediate_action_mask |= genericpoints::PTF_REV | genericpoints::PTF_OOC;

			//code for random failures goes here

			ActionRegisterFuture(std::make_shared<future_pointsaction>(*target, GetPointsMovementCompletionTime(), index, 0, genericpoints::PTF_OOC));
		}
	}
	if(change_flags & genericpoints::PTF_LOCKED) {
		immediate_action_bits |= bits & genericpoints::PTF_LOCKED;
		immediate_action_mask |= genericpoints::PTF_LOCKED;
	}
	if(change_flags & genericpoints::PTF_REMINDER) {
		immediate_action_bits |= bits & genericpoints::PTF_REMINDER;
		immediate_action_mask |= genericpoints::PTF_REMINDER;
	}

	ActionRegisterFuture(std::make_shared<future_pointsaction>(*target, w.GetGameTime() + 1, index, immediate_action_bits, immediate_action_mask));
}

world_time action_pointsaction::GetPointsMovementCompletionTime() const {
	return action_time + 5000;
}

void action_pointsaction::CancelFutures(unsigned int index, unsigned int setmask, unsigned int clearmask) const {
	auto func = [&](future &f) {
		future_pointsaction *fp = dynamic_cast<future_pointsaction *>(&f);
		if(!fp) return;
		if(fp->index != index) return;
		unsigned int foundset = fp->mask & fp->bits & setmask;
		unsigned int foundunset = fp->mask & (~fp->bits) & clearmask;
		if(foundset || foundunset) {
			unsigned int newmask = fp->mask & ~(foundset | foundunset);
			if(newmask) {
				ActionRegisterFuture(std::make_shared<future_pointsaction>(*target, fp->GetTriggerTime(), index, fp->bits, newmask));
			}
			ActionCancelFuture(f);
		}
	};

	target->EnumerateFutures(func);
}

void action_pointsaction::Deserialise(const deserialiser_input &di, error_collection &ec) {
	CheckTransJsonValueDef(index, di, "index", 0, ec);
	CheckTransJsonValueDef(bits, di, "bits", 0, ec);
	CheckTransJsonValueDef(mask, di, "mask", 0, ec);
}

void action_pointsaction::Serialise(serialiser_output &so, error_collection &ec) const {
	SerialiseValueJson(index, so, "index");
	SerialiseValueJson(bits, so, "bits");
	SerialiseValueJson(mask, so, "mask");
}

void future_pointsactionmessage::PrepareVariables(message_formatter &mf, world &w) {
	mf.RegisterVariable("reason", [&](const std::string &in) { return w.GetUserMessageTextpool().GetTextByName(this->reasonkey); });

	genericpoints *gp = dynamic_cast<genericpoints *>(&GetTarget());
	if(gp) mf.RegisterVariable("points", [=](const std::string &in) { return gp->GetName(); });
}

void future_pointsactionmessage::Deserialise(const deserialiser_input &di, error_collection &ec) {
	future_genericusermessage::Deserialise(di, ec);
	CheckTransJsonValue(reasonkey, di, "reasonkey", ec);
}

void future_pointsactionmessage::Serialise(serialiser_output &so, error_collection &ec) const {
	future_genericusermessage::Serialise(so, ec);
	SerialiseValueJson(reasonkey, so, "reasonkey");
}

//return true on success
bool action_reservetrack_base::TryReserveRoute(route *rt, world_time action_time) const {
	if(!rt->start.track->Reservation(rt->start.direction, 0, RRF_TRYRESERVE | RRF_STARTPIECE, rt)) return false;
	for(auto it = rt->pieces.begin(); it != rt->pieces.end(); ++it) {
		if(!it->location.track->Reservation(it->location.direction, it->connection_index, RRF_TRYRESERVE, rt)) return false;
	}
	if(!rt->end.track->Reservation(rt->end.direction, 0, RRF_TRYRESERVE | RRF_ENDPIECE, rt)) return false;
	
	//route is OK, now reserve it
	
	auto actioncallback = [&](action &&reservation_act) {
		reservation_act.action_time++;
		reservation_act.Execute();
	};
	
	rt->start.track->Reservation(rt->start.direction, 0, RRF_RESERVE | RRF_STARTPIECE, rt);
	rt->start.track->ReservationActions(rt->start.direction, 0, RRF_RESERVE | RRF_STARTPIECE, rt, w, actioncallback);
	for(auto it = rt->pieces.begin(); it != rt->pieces.end(); ++it) {
		it->location.track->Reservation(it->location.direction, it->connection_index, RRF_TRYRESERVE, rt);
		it->location.track->ReservationActions(it->location.direction, it->connection_index, RRF_TRYRESERVE, rt, w, actioncallback);
	}
	rt->end.track->Reservation(rt->end.direction, 0, RRF_RESERVE | RRF_STARTPIECE, rt);
	rt->end.track->ReservationActions(rt->end.direction, 0, RRF_RESERVE | RRF_ENDPIECE, rt, w, actioncallback);
	return true;
}

void action_reservetrack::ExecuteAction() const {
	if(!target) return;

	if(!TryReserveRoute(target, action_time)) {
		ActionSendReplyFuture(std::make_shared<future_genericusermessage>(w, action_time+1, &w, "track/reservation/fail"));
	}
}

void action_reservetrack::Deserialise(const deserialiser_input &di, error_collection &ec) {
	std::string routeparentname;
	unsigned int index;
	if(CheckTransJsonValue(routeparentname, di, "routeparent", ec) && CheckTransJsonValue(index, di, "routeindex", ec)) {
		routingpoint *rp = dynamic_cast<routingpoint *>(w.FindTrackByName(routeparentname));
		if(rp) target = rp->GetRouteByIndex(index);
	}

	if(!target) ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation(di, "Invalid track reservation action definition")));
}

void action_reservetrack::Serialise(serialiser_output &so, error_collection &ec) const {
	SerialiseValueJson(target->parent->GetSerialisationName(), so, "routeparent");
	SerialiseValueJson(target->index, so, "routeindex");
}
