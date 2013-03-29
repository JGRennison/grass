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

#include <memory>
#include <algorithm>
#include "common.h"
#include "track_ops.h"
#include "track.h"
#include "points.h"
#include "serialisable_impl.h"
#include "signal.h"
#include "textpool.h"
#include "trackcircuit.h"


void future_pointsaction::ExecuteAction() {
	genericpoints *gp = dynamic_cast<genericpoints *>(&GetTarget());
	if(gp) {
		gp->SetPointFlagsMasked(index, bits, mask);
	}
}

void future_pointsaction::Deserialise(const deserialiser_input &di, error_collection &ec) {
	future::Deserialise(di, ec);
	CheckTransJsonValueDef(index, di, "index", 0, ec);
	CheckTransJsonValueDef(bits, di, "bits", genericpoints::PTF::ZERO, ec);
	CheckTransJsonValueDef(mask, di, "mask", genericpoints::PTF::ZERO, ec);
}

void future_pointsaction::Serialise(serialiser_output &so, error_collection &ec) const {
	future::Serialise(so, ec);
	SerialiseValueJson(index, so, "index");
	SerialiseValueJson(bits, so, "bits");
	SerialiseValueJson(mask, so, "mask");
}

void action_pointsaction::ExecuteAction() const {
	if(!target) return;

	genericpoints::PTF old_pflags = target->GetPointFlags(index);
	genericpoints::PTF change_flags = (old_pflags ^ bits) & mask;

	genericpoints::PTF immediate_action_bits = genericpoints::PTF::ZERO;
	genericpoints::PTF immediate_action_mask = genericpoints::PTF::ZERO;

	std::function<void()> overlap_callback;

	if(change_flags & genericpoints::PTF::REV) {
		if(old_pflags & genericpoints::PTF::LOCKED) {
			ActionSendReplyFuture(std::make_shared<future_genericusermessage_reason>(*target, action_time+1, &w, "track_ops/pointsunmovable", "points/locked"));
			return;
		}
		if(old_pflags & genericpoints::PTF::REMINDER) {
			ActionSendReplyFuture(std::make_shared<future_genericusermessage_reason>(*target, action_time+1, &w, "track_ops/pointsunmovable", "points/reminderset"));
			return;
		}
		if(!(aflags & APAF::IGNORERESERVATION) && target->GetFlags(target->GetDefaultValidDirecton()) & GTF::ROUTESET) {
			std::string failreason = "track/reserved";
			if(!TrySwingOverlap(overlap_callback, !!(bits & genericpoints::PTF::REV), &failreason)) {
				ActionSendReplyFuture(std::make_shared<future_genericusermessage_reason>(*target, action_time+1, &w, "track_ops/pointsunmovable", failreason));
				return;
			}
		}

		CancelFutures(index, genericpoints::PTF::ZERO, genericpoints::PTF::OOC);
		immediate_action_bits |= (bits & genericpoints::PTF::REV) | genericpoints::PTF::OOC;
		immediate_action_mask |= genericpoints::PTF::REV | genericpoints::PTF::OOC;

		//code for random failures goes here

		ActionRegisterFuture(std::make_shared<future_pointsaction>(*target, GetPointsMovementCompletionTime(), index, genericpoints::PTF::ZERO, genericpoints::PTF::OOC));
	}
	genericpoints::PTF immediate_change_flags = change_flags & (genericpoints::PTF::LOCKED | genericpoints::PTF::REMINDER);
	immediate_action_mask |= immediate_change_flags;
	immediate_action_bits |= bits & immediate_change_flags;

	ActionRegisterFuture(std::make_shared<future_pointsaction>(*target, action_time + 1, index, immediate_action_bits, immediate_action_mask));

	if(overlap_callback) overlap_callback();
}

world_time action_pointsaction::GetPointsMovementCompletionTime() const {
	return action_time + 5000;
}

void action_pointsaction::CancelFutures(unsigned int index, genericpoints::PTF setmask, genericpoints::PTF clearmask) const {
	auto func = [&](future &f) {
		future_pointsaction *fp = dynamic_cast<future_pointsaction *>(&f);
		if(!fp) return;
		if(fp->index != index) return;
		genericpoints::PTF foundset = fp->mask & fp->bits & setmask;
		genericpoints::PTF foundunset = fp->mask & (~fp->bits) & clearmask;
		if(foundset || foundunset) {
			flagwrapper<genericpoints::PTF> newmask = fp->mask & ~(foundset | foundunset);
			if(newmask) {
				ActionRegisterFuture(std::make_shared<future_pointsaction>(*target, fp->GetTriggerTime(), index, fp->bits, newmask));
			}
			ActionCancelFuture(f);
		}
	};

	target->EnumerateFutures(func);
}

bool action_pointsaction::TrySwingOverlap(std::function<void()> &overlap_callback, bool settingreverse, std::string *failreasonkey) const {
	if(aflags & APAF::NOOVERLAPSWING) return false;

	std::vector<track_reservation_state *> pointstrslist;
	target->GetTRSList(pointstrslist);
	const route *foundoverlap = 0;
	bool failed = false;
	for(auto it = pointstrslist.begin(); it != pointstrslist.end(); ++it) {
		(*it)->ReservationEnumeration([&](const route *reserved_route, EDGETYPE direction, unsigned int index, RRF rr_flags) {
			if(! (rr_flags & RRF::RESERVE)) return;
			if(reserved_route->type != RTC_OVERLAP) {
				failed = true;
				return;
			}
			if(foundoverlap) {
				failed = true;
				return;
			}
			foundoverlap = reserved_route;
		});
	}
	if(failed || !foundoverlap) return false;

	genericsignal *start = dynamic_cast<genericsignal*>(foundoverlap->start.track);
	if(!start) return false;
	if(!start->IsOverlapSwingPermitted(failreasonkey)) return false;

	//temporarily "move" points
	genericpoints::PTF saved_pflags = target->SetPointFlagsMasked(index, settingreverse ? genericpoints::PTF::REV : genericpoints::PTF::ZERO, genericpoints::PTF::REV);

	const route *best_new_overlap = 0;
	int best_overlap_score = INT_MIN;
	start->EnumerateAvailableOverlaps([&](const route *rt, int score) {
		if(score > best_overlap_score) {
			best_new_overlap = rt;
			best_overlap_score = score;
		}
	}, RRF::IGNORE_OWN_OVERLAP);

	//move back
	target->SetPointFlagsMasked(index, saved_pflags, ~genericpoints::PTF::ZERO);

	bool result = false;

	if(best_new_overlap && best_new_overlap != foundoverlap) {
		overlap_callback = [=]() {
			auto actioncallback = [&](action &&reservation_act) {
				reservation_act.Execute();
			};
			if(foundoverlap) {
				foundoverlap->RouteReservationActions(RRF::UNRESERVE, actioncallback);
				ActionRegisterFuture(std::make_shared<future_unreservetrack>(*start, action_time + 1, foundoverlap));
			}
			if(best_new_overlap) {
				best_new_overlap->RouteReservationActions(RRF::RESERVE, actioncallback);
				ActionRegisterFuture(std::make_shared<future_reservetrack>(*start, action_time + 1, best_new_overlap));
				best_new_overlap->RouteReservation(RRF::PROVISIONAL_RESERVE | RRF::IGNORE_OWN_OVERLAP);
			}
		};
		result = true;
	}

	return result;
}

void action_pointsaction::Deserialise(const deserialiser_input &di, error_collection &ec) {
	CheckTransJsonValueDef(index, di, "index", 0, ec);
	CheckTransJsonValueDef(bits, di, "bits", genericpoints::PTF::ZERO, ec);
	CheckTransJsonValueDef(mask, di, "mask", genericpoints::PTF::ZERO, ec);
}

void action_pointsaction::Serialise(serialiser_output &so, error_collection &ec) const {
	SerialiseValueJson(index, so, "index");
	SerialiseValueJson(bits, so, "bits");
	SerialiseValueJson(mask, so, "mask");
}

void future_routeoperation_base::Deserialise(const deserialiser_input &di, error_collection &ec) {
	future::Deserialise(di, ec);
	DeserialiseRouteTargetByParentAndIndex(reserved_route, di, ec, false);
}

void future_routeoperation_base::Serialise(serialiser_output &so, error_collection &ec) const {
	future::Serialise(so, ec);
	if(reserved_route) {
		if(reserved_route->parent) {
			SerialiseValueJson(reserved_route->parent->GetName(), so, "route_parent");
			SerialiseValueJson(reserved_route->index, so, "route_index");
		}
	}
}

void future_reservetrack_base::ExecuteAction() {
	if(reserved_route) {
		reserved_route->RouteReservation(rflags);
	}
}

void future_reservetrack_base::Deserialise(const deserialiser_input &di, error_collection &ec) {
	future_routeoperation_base::Deserialise(di, ec);
	CheckTransJsonValueDef(rflags, di, "rflags", RRF::ZERO, ec);
}

void future_reservetrack_base::Serialise(serialiser_output &so, error_collection &ec) const {
	future_routeoperation_base::Serialise(so, ec);
	SerialiseValueJson(rflags, so, "rflags");
}

const route *action_reservetrack_base::TestSwingOverlapAndReserve(const route *target_route, std::string *failreasonkey) const {
	genericsignal *gs = dynamic_cast<genericsignal*>(target_route->start.track);
	if(!gs) return 0;
	if(!target_route->RouteReservation(RRF::TRYRESERVE | RRF::IGNORE_OWN_OVERLAP)) return 0;	//can't reserve even when ignoring the overlap

	if(!gs->IsOverlapSwingPermitted(failreasonkey)) return 0;

	std::vector<std::pair<int, const route *> > candidates;
	gs->EnumerateAvailableOverlaps([&](const route *rt, int score) {
		candidates.push_back(std::make_pair(-score, rt));
	}, RRF::IGNORE_OWN_OVERLAP);
	std::sort(candidates.begin(), candidates.end());

	const route *result = 0;
	for(auto it = candidates.begin(); it != candidates.end(); ++it) {
		if(target_route->IsRouteSubSet(it->second)) {
			result = it->second;
			break;
		}
	}
	return result;
}

//return true on success
bool action_reservetrack_base::TryReserveRoute(const route *rt, world_time action_time, std::function<void(const std::shared_ptr<future> &f)> error_handler) const {
	//disallow if non-overlap route already set from start point in given direction
	if(rt->start.track->GetSetRouteTypes(rt->start.direction) & (RPRT::MASK_START & ~RPRT::OVERLAPSTART)) {
		error_handler(std::make_shared<future_genericusermessage_reason>(w, action_time+1, &w, "track/reservation/fail", "track/reservation/alreadyset"));
		return false;
	}

	const route *swing_this_overlap = 0;
	const route *remove_prev_overlap = 0;
	std::string tryreservation_failreasonkey = "generic/failurereason";
	bool success = rt->RouteReservation(RRF::TRYRESERVE, &tryreservation_failreasonkey);
	if(!success) {
		swing_this_overlap = TestSwingOverlapAndReserve(rt, &tryreservation_failreasonkey);
		if(swing_this_overlap) {
			success = true;
			genericsignal *sig = dynamic_cast<genericsignal*>(rt->start.track);
			if(sig) remove_prev_overlap = sig->GetCurrentForwardOverlap();
		}
	}

	if(!success) {
		error_handler(std::make_shared<future_genericusermessage_reason>(w, action_time+1, &w, "track/reservation/fail", tryreservation_failreasonkey));
		return false;
	}

	const route *best_overlap = 0;
	if(rt->routeflags & route::RF::NEEDOVERLAP) {
		//need an overlap too
		best_overlap = rt->end.track->FindBestOverlap();
		if(!best_overlap) {
			error_handler(std::make_shared<future_genericusermessage_reason>(w, action_time+1, &w, "track/reservation/fail", "track/reservation/overlap/noneavailable"));
			return false;
		}
	}

	//route is OK, now reserve it

	auto actioncallback = [&](action &&reservation_act) {
		action_pointsaction *apa = dynamic_cast<action_pointsaction*>(&reservation_act);
		if(apa) apa->SetFlagsMasked(action_pointsaction::APAF::IGNORERESERVATION, action_pointsaction::APAF::IGNORERESERVATION);
		reservation_act.Execute();
	};

	if(remove_prev_overlap) {
		remove_prev_overlap->RouteReservationActions(RRF::UNRESERVE, actioncallback);
		ActionRegisterFuture(std::make_shared<future_unreservetrack>(*rt->start.track, action_time + 1, remove_prev_overlap));
	}
	if(swing_this_overlap) {
		swing_this_overlap->RouteReservationActions(RRF::RESERVE, actioncallback);
		ActionRegisterFuture(std::make_shared<future_reservetrack>(*rt->start.track, action_time + 1, swing_this_overlap));
		swing_this_overlap->RouteReservation(RRF::PROVISIONAL_RESERVE | RRF::IGNORE_OWN_OVERLAP);
	}
	rt->RouteReservationActions(RRF::RESERVE, actioncallback);
	ActionRegisterFuture(std::make_shared<future_reservetrack>(*rt->start.track, action_time + 1, rt));
	rt->RouteReservation(RRF::PROVISIONAL_RESERVE);
	if(best_overlap) {
		best_overlap->RouteReservationActions(RRF::RESERVE, actioncallback);
		ActionRegisterFuture(std::make_shared<future_reservetrack>(*rt->start.track, action_time + 1, best_overlap));
		best_overlap->RouteReservation(RRF::PROVISIONAL_RESERVE);
	}
	return true;
}

//return true on success
bool action_reservetrack_base::TryUnreserveRoute(routingpoint *startsig, world_time action_time, std::function<void(const std::shared_ptr<future> &f)> error_handler) const {
	genericsignal *sig = dynamic_cast<genericsignal*>(startsig);
	if(!sig) {
		error_handler(std::make_shared<future_genericusermessage_reason>(w, action_time+1, &w, "track/unreservation/fail", "track/reservation/notsignal"));
		return false;
	}

	if(sig->GetSignalFlags() & genericsignal::GSF::AUTOSIGNAL) {
		error_handler(std::make_shared<future_genericusermessage_reason>(w, action_time+1, &w, "track/unreservation/fail", "track/unreservation/autosignal"));
		return false;
	}

	const route *rt = sig->GetCurrentForwardRoute();
	if(!rt) {
		error_handler(std::make_shared<future_genericusermessage_reason>(w, action_time+1, &w, "track/unreservation/fail", "track/notreserved"));
		return false;
	}

	//approach locking checks
	if(sig->GetAspect() > 0) {

		bool approachlocking_engage = false;
		unsigned int backaspect = 0;

		auto checksignal = [&](const genericsignal *gs) -> bool {
			bool aspectchange = gs->GetAspect() > backaspect;	//cancelling route would reduce signal aspect
			backaspect++;
			return aspectchange;
		};

		auto checkpiece = [&](const track_target_ptr &piece) -> bool {
			track_circuit *tc = piece.track->GetTrackCircuit();
			if(tc && tc->Occupied()) {
				approachlocking_engage = true;		//found an occupied piece, train is on approach
				return false;
			}
			else return true;
		};

		sig->BackwardsReservedTrackScan(checksignal, checkpiece);

		if(approachlocking_engage) {
			//code goes here
		}
	}

	//now unreserve route

	GenericRouteUnreservation(rt, sig, RRF::STOP_ON_OCCUPIED_TC);

	return true;
}

bool action_reservetrack_base::GenericRouteUnreservation(const route *targrt, routingpoint *targsig, RRF extraflags) const {
	bool fullyunreserved = targrt->RouteReservation(RRF::TRYUNRESERVE | extraflags);

	ActionRegisterFuture(std::make_shared<future_unreservetrack>(*targsig, action_time + 1, targrt, extraflags));

	auto actioncallback = [&](action &&reservation_act) {
		reservation_act.action_time++;
		reservation_act.Execute();
	};
	targrt->RouteReservationActions(RRF::UNRESERVE | extraflags, actioncallback);
	return fullyunreserved;
}

void action_routereservetrackop::Deserialise(const deserialiser_input &di, error_collection &ec) {
	DeserialiseRouteTargetByParentAndIndex(targetroute, di, ec, false);

	if(!targetroute) ec.RegisterNewError<error_deserialisation>(di, "Invalid track reservation action definition");
}

void action_routereservetrackop::Serialise(serialiser_output &so, error_collection &ec) const {
	SerialiseValueJson(targetroute->parent->GetSerialisationName(), so, "routeparent");
	SerialiseValueJson(targetroute->index, so, "routeindex");
}

void action_reservetrack::ExecuteAction() const {
	if(!targetroute) return;

	TryReserveRoute(targetroute, action_time, [&](const std::shared_ptr<future> &f) {
		ActionSendReplyFuture(f);
	});
}

action_reservepath::action_reservepath(world &w_, const routingpoint *start_, const routingpoint *end_)
: action_reservetrack_base(w_), start(start_), end(end_), gmr_flags(GMRF::ROUTEOK | GMRF::SHUNTOK), extraflags(RRF::IGNORE_OWN_OVERLAP) { }
action_reservepath &action_reservepath::SetGmrFlags(GMRF gmr_flags_) { gmr_flags = gmr_flags_; return *this; }
action_reservepath &action_reservepath::SetExtraFlags(RRF extraflags_) { extraflags = extraflags_; return *this; }
action_reservepath &action_reservepath::SetVias(via_list vias_) { vias = vias_; return *this; }

void action_reservepath::ExecuteAction() const {
	if(!start) {
		ActionSendReplyFuture(std::make_shared<future_genericusermessage_reason>(w, action_time+1, &w, "track/reservation/fail", "track/reservation/notsignal"));
		return;
	}

	std::vector<routingpoint::gmr_routeitem> routes;
	unsigned int routecount = start->GetMatchingRoutes(routes, end, gmr_flags, extraflags, vias);

	if(!routecount) {
		ActionSendReplyFuture(std::make_shared<future_genericusermessage_reason>(w, action_time+1, &w, "track/reservation/fail", "track/reservation/noroute"));
		return;
	}

	std::vector<std::shared_ptr<future> > failmessages;

	for(auto it = routes.begin(); it != routes.end(); ++it) {
		bool success = TryReserveRoute(it->rt, action_time, [&](const std::shared_ptr<future> &f) {
			if(it == routes.begin()) failmessages.push_back(f);
		});
		if(success) return;
	}
	for(auto it = failmessages.begin(); it != failmessages.end(); ++it) {
		ActionSendReplyFuture(*it);
	}
}

void action_reservepath::Deserialise(const deserialiser_input &di, error_collection &ec) {
	std::string targetname;
	if(CheckTransJsonValue(targetname, di, "start", ec)) {
		start = dynamic_cast<routingpoint *>(w.FindTrackByName(targetname));
	}
	if(CheckTransJsonValue(targetname, di, "end", ec)) {
		end = dynamic_cast<routingpoint *>(w.FindTrackByName(targetname));
	}
	CheckTransJsonValue(gmr_flags, di, "gmr_flags", ec);
	CheckTransJsonValue(extraflags, di, "extraflags", ec);

	auto viaparser = [&](const deserialiser_input &di, error_collection &ec) {
		routingpoint *rp = dynamic_cast<routingpoint *>(w.FindTrackByName(GetType<std::string>(di.json)));
		if(rp) vias.push_back(rp);
	};
	vias.clear();
	CheckIterateJsonArrayOrType<std::string>(di, "vias", "", ec, viaparser);

	if(!start) ec.RegisterNewError<error_deserialisation>(di, "Invalid path reservation action definition");
}

void action_reservepath::Serialise(serialiser_output &so, error_collection &ec) const {
	SerialiseValueJson(start->GetSerialisationName(), so, "start");
	SerialiseValueJson(end->GetSerialisationName(), so, "end");
	SerialiseValueJson(gmr_flags, so, "gmr_flags");
	SerialiseValueJson(extraflags, so, "extraflags");
	if(vias.size()) {
		so.json_out.String("vias");
		so.json_out.StartArray();
		for(auto it = vias.begin(); it != vias.end(); ++it) {
			so.json_out.String((*it)->GetSerialisationName());
		}
		so.json_out.EndArray();
	}
}

void action_unreservetrack::ExecuteAction() const {
	if(!target) return;
	const route *rt = target->GetCurrentForwardRoute();
	if(rt) GenericRouteUnreservation(rt, rt->start.track, RRF::STOP_ON_OCCUPIED_TC);
}

void action_unreservetrack::Deserialise(const deserialiser_input &di, error_collection &ec) {
	std::string targetname;
	if(CheckTransJsonValue(targetname, di, "target", ec)) {
		target = dynamic_cast<genericsignal *>(w.FindTrackByName(targetname));
	}

	if(!target) ec.RegisterNewError<error_deserialisation>(di, "Invalid track unreservation action definition");
}

void action_unreservetrack::Serialise(serialiser_output &so, error_collection &ec) const {
	SerialiseValueJson(target->GetSerialisationName(), so, "target");
}

void action_unreservetrackroute::ExecuteAction() const {
	GenericRouteUnreservation(targetroute, targetroute->start.track, RRF::STOP_ON_OCCUPIED_TC);
}