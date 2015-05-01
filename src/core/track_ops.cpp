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

#include <memory>
#include <algorithm>
#include "common.h"
#include "core/trackreservation.h"
#include "core/track_ops.h"
#include "core/track.h"
#include "core/points.h"
#include "core/serialisable_impl.h"
#include "core/signal.h"
#include "core/textpool.h"
#include "core/trackcircuit.h"
#include "core/routetypes_serialisation.h"


void future_pointsaction::ExecuteAction() {
	genericpoints *gp = dynamic_cast<genericpoints *>(&GetTarget());
	if(gp) {
		gp->SetPointsFlagsMasked(index, bits, mask);
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

// Generate flag change for a nominal normalise and reverse operation
action_pointsaction::action_pointsaction(world &w_, genericpoints &targ, unsigned int index_, bool reverse)
		: action(w_), target(&targ), index(index_) {
	genericpoints::PTF pflags = target->GetPointsFlags(index);
	if(static_cast<bool>(pflags & genericpoints::PTF::REV) != reverse) {
		if(pflags & genericpoints::PTF::LOCKED) {
			// unlock
			bits = genericpoints::PTF::ZERO;
			mask = genericpoints::PTF::LOCKED;
		}
		else {
			// Change position
			bits = SetOrClearBits<genericpoints::PTF>(genericpoints::PTF::ZERO, genericpoints::PTF::REV, reverse);
			mask = genericpoints::PTF::REV;
		}
	}
	else {
		// Set locked state
		bits = genericpoints::PTF::LOCKED;
		mask = genericpoints::PTF::LOCKED;
	}
}

void action_pointsaction::ExecuteAction() const {
	if(!target)
		return;

	genericpoints::PTF old_pflags = target->GetPointsFlags(index);
	genericpoints::PTF change_flags = (old_pflags ^ bits) & mask;

	genericpoints::PTF immediate_action_bits = genericpoints::PTF::ZERO;
	genericpoints::PTF immediate_action_mask = genericpoints::PTF::ZERO;

	std::function<void()> overlap_callback;

	if(change_flags & genericpoints::PTF::REV) {
		if(old_pflags & genericpoints::PTF::LOCKED) {
			ActionSendReplyFuture(std::make_shared<future_genericusermessage_reason>(*target, action_time+1, &w,
					"track_ops/pointsunmovable", "points/locked"));
			return;
		}
		if(old_pflags & genericpoints::PTF::REMINDER) {
			ActionSendReplyFuture(std::make_shared<future_genericusermessage_reason>(*target, action_time+1, &w,
					"track_ops/pointsunmovable", "points/reminderset"));
			return;
		}
		if(!(aflags & APAF::IGNORERESERVATION) && target->GetFlags(target->GetDefaultValidDirecton()) & GTF::ROUTESET) {
			std::string failreason = "track/reserved";
			if(!TrySwingOverlap(overlap_callback, !!(bits & genericpoints::PTF::REV), &failreason)) {
				ActionSendReplyFuture(std::make_shared<future_genericusermessage_reason>(*target, action_time+1, &w,
						"track_ops/pointsunmovable", failreason));
				return;
			}
		}

		CancelFutures(index, genericpoints::PTF::ZERO, genericpoints::PTF::OOC);
		immediate_action_bits |= (bits & genericpoints::PTF::REV) | genericpoints::PTF::OOC;
		immediate_action_mask |= genericpoints::PTF::REV | genericpoints::PTF::OOC;

		//code for random failures goes here

		ActionRegisterFuture(std::make_shared<future_pointsaction>(*target, GetPointsMovementCompletionTime(), index,
				genericpoints::PTF::ZERO, genericpoints::PTF::OOC));
	}
	genericpoints::PTF immediate_change_flags = change_flags & (genericpoints::PTF::LOCKED | genericpoints::PTF::REMINDER);
	immediate_action_mask |= immediate_change_flags;
	immediate_action_bits |= bits & immediate_change_flags;

	ActionRegisterFuture(std::make_shared<future_pointsaction>(*target, action_time + 1, index, immediate_action_bits, immediate_action_mask));

	if(overlap_callback)
		overlap_callback();
}

world_time action_pointsaction::GetPointsMovementCompletionTime() const {
	return action_time + 5000;
}

void action_pointsaction::CancelFutures(unsigned int index, genericpoints::PTF setmask, genericpoints::PTF clearmask) const {

	auto check_instance = [&](genericpoints *p, unsigned int index, genericpoints::PTF setmask, genericpoints::PTF clearmask) {
		auto func = [&](future &f) {
			future_pointsaction *fp = dynamic_cast<future_pointsaction *>(&f);
			if(!fp)
				return;
			if(fp->index != index)
				return;
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

		p->EnumerateFutures(func);
	};
	check_instance(target, index, setmask, clearmask);
	std::vector<genericpoints::points_coupling> *coupling = target->GetCouplingVector(index);
	if(coupling) {
		for(auto &it : *coupling) {
			genericpoints::PTF rev = (setmask ^ clearmask) & it.xormask;
			check_instance(it.targ, it.index, setmask ^ rev, clearmask ^ rev);
		}
	}
}

bool action_pointsaction::TrySwingOverlap(std::function<void()> &overlap_callback, bool settingreverse, std::string *failreasonkey) const {
	if(aflags & APAF::NOOVERLAPSWING) return false;

	const route *foundoverlap = nullptr;
	bool failed = false;
	target->ReservationEnumeration([&](const route *reserved_route, EDGETYPE direction, unsigned int index, RRF rr_flags) {
		if(!route_class::IsOverlap(reserved_route->type)) {
			failed = true;
			return;
		}
		if(foundoverlap) {
			failed = true;
			return;
		}
		foundoverlap = reserved_route;
	}, RRF::RESERVE | RRF::PROVISIONAL_RESERVE);
	if(failed || !foundoverlap)
		return false;

	genericsignal *start = FastSignalCast(foundoverlap->start.track, foundoverlap->start.direction);
	if(!start)
		return false;
	if(!start->IsOverlapSwingPermitted(failreasonkey))
		return false;

	//temporarily "move" points
	genericpoints::PTF saved_pflags = target->SetPointsFlagsMasked(index, settingreverse ? genericpoints::PTF::REV : genericpoints::PTF::ZERO, genericpoints::PTF::REV);

	const route *best_new_overlap = nullptr;
	int best_overlap_score = INT_MIN;
	start->EnumerateAvailableOverlaps([&](const route *rt, int score) {
		if(score > best_overlap_score) {
			best_new_overlap = rt;
			best_overlap_score = score;
		}
	}, RRF::IGNORE_OWN_OVERLAP);

	//move back
	target->SetPointsFlagsMasked(index, saved_pflags, ~genericpoints::PTF::ZERO);

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
	action::Deserialise(di, ec);

	std::string targetname;
	if(CheckTransJsonValue(targetname, di, "target", ec))
		target = w.FindTrackByNameCast<genericpoints>(targetname);

	if(!target)
		ec.RegisterNewError<error_deserialisation>(di, "Invalid action_pointsaction definition");

	CheckTransJsonValueDef(index, di, "index", 0, ec);
	CheckTransJsonValueDef(bits, di, "bits", genericpoints::PTF::ZERO, ec);
	CheckTransJsonValueDef(mask, di, "mask", genericpoints::PTF::ZERO, ec);
}

void action_pointsaction::Serialise(serialiser_output &so, error_collection &ec) const {
	action::Serialise(so, ec);
	SerialiseValueJson(target->GetSerialisationName(), so, "target");
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
	genericsignal *gs = FastSignalCast(target_route->start.track, target_route->start.direction);
	if(!gs)
		return nullptr;
	if(!target_route->RouteReservation(RRF::TRYRESERVE | RRF::IGNORE_OWN_OVERLAP))
		return nullptr;    //can't reserve even when ignoring the overlap

	if(!gs->IsOverlapSwingPermitted(failreasonkey))
		return nullptr;

	std::vector<std::pair<int, const route *> > candidates;
	gs->EnumerateAvailableOverlaps([&](const route *rt, int score) {
		candidates.push_back(std::make_pair(-score, rt));
	}, RRF::IGNORE_OWN_OVERLAP);
	std::sort(candidates.begin(), candidates.end());

	const route *result = nullptr;
	for(auto &it : candidates) {
		if(target_route->IsRouteSubSet(it.second)) {
			result = it.second;
			break;
		}
	}
	return result;
}

//return true on success
bool action_reservetrack_base::TryReserveRoute(const route *rt, world_time action_time,
		std::function<void(const std::shared_ptr<future> &f)> error_handler) const {
	//disallow if non-overlap route already set from start point in given direction
	//but silently accept if the set route is identical to the one trying to be set
	bool found_route = false;
	bool route_conflict = false;
	rt->start.track->ReservationEnumerationInDirection(rt->start.direction,
			[&](const route *reserved_route, EDGETYPE direction, unsigned int index, RRF rr_flags) {
		if(route_class::IsOverlap(reserved_route->type))
			return;
		if(!(rr_flags & RRF::STARTPIECE))
			return;
		found_route = true;
		if(reserved_route != rt)
			route_conflict = true;
	}, RRF::RESERVE | RRF::PROVISIONAL_RESERVE);
	if(found_route) {
		if(route_conflict) {
			error_handler(std::make_shared<future_genericusermessage_reason>(w, action_time + 1, &w, "track/reservation/fail", "track/reservation/alreadyset"));
			return false;
		}
		else {
			genericsignal *sig = FastSignalCast(rt->start.track, rt->start.direction);
			if(sig)
				CancelApproachLocking(sig);
			return true;
		}
	}

	const route *swing_this_overlap = nullptr;
	const route *remove_prev_overlap = nullptr;
	std::string tryreservation_failreasonkey = "generic/failurereason";
	bool success = rt->RouteReservation(RRF::TRYRESERVE, &tryreservation_failreasonkey);
	if(!success) {
		swing_this_overlap = TestSwingOverlapAndReserve(rt, &tryreservation_failreasonkey);
		if(swing_this_overlap) {
			success = true;
			genericsignal *sig = FastSignalCast(rt->start.track, rt->start.direction);
			if(sig)
				remove_prev_overlap = sig->GetCurrentForwardOverlap();
		}
	}

	if(!success) {
		error_handler(std::make_shared<future_genericusermessage_reason>(w, action_time + 1, &w,
				"track/reservation/fail", tryreservation_failreasonkey));
		return false;
	}

	const route *best_overlap = nullptr;
	if(rt->overlap_type != route_class::ID::RTC_NULL) {
		//need an overlap too
		best_overlap = rt->end.track->FindBestOverlap(route_class::Flag(rt->overlap_type));
		if(!best_overlap) {
			error_handler(std::make_shared<future_genericusermessage_reason>(w, action_time + 1, &w,
					"track/reservation/fail", "track/reservation/overlap/noneavailable"));
			return false;
		}
	}

	//route is OK, now reserve it

	auto actioncallback = [&](action &&reservation_act) {
		action_pointsaction *apa = dynamic_cast<action_pointsaction*>(&reservation_act);
		if(apa)
			apa->SetFlagsMasked(action_pointsaction::APAF::IGNORERESERVATION, action_pointsaction::APAF::IGNORERESERVATION);
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
bool action_reservetrack_base::TryUnreserveRoute(routingpoint *startsig, world_time action_time,
		std::function<void(const std::shared_ptr<future> &f)> error_handler) const {
	genericsignal *sig = FastSignalCast(startsig);
	if(!sig) {
		error_handler(std::make_shared<future_genericusermessage_reason>(w, action_time + 1, &w,
				"track/unreservation/fail", "track/reservation/notsignal"));
		return false;
	}

	if(sig->GetSignalFlags() & GSF::AUTOSIGNAL) {
		error_handler(std::make_shared<future_genericusermessage_reason>(w, action_time + 1, &w,
				"track/unreservation/fail", "track/unreservation/autosignal"));
		return false;
	}

	const route *rt = sig->GetCurrentForwardRoute();
	if(!rt) {
		error_handler(std::make_shared<future_genericusermessage_reason>(w, action_time + 1, &w,
				"track/unreservation/fail", "track/notreserved"));
		return false;
	}

	if(sig->GetSignalFlags() & GSF::APPROACHLOCKINGMODE)
		return true;    //don't try to unreserve again

	//approach locking checks
	if(sig->GetAspect() > 0) {
		bool approachlocking_engage = false;
		unsigned int backaspect = 0;

		auto checksignal = [&](const genericsignal *gs) -> bool {
			bool aspectchange = gs->GetReservedAspect() > backaspect;
				//cancelling route would reduce signal aspect
				//this includes existing approach control aspects

			backaspect++;
			return aspectchange;
		};

		auto checkpiece = [&](const track_target_ptr &piece) -> bool {
			track_circuit *tc = piece.track->GetTrackCircuit();
			if(tc && tc->Occupied()) {
				approachlocking_engage = true;    //found an occupied piece, train is on approach
				return false;
			}
			else {
				return true;
			}
		};

		sig->BackwardsReservedTrackScan(checksignal, checkpiece);

		if(approachlocking_engage) {
			ActionRegisterFuture(std::make_shared<future_signalflags>(*sig, action_time+1, GSF::APPROACHLOCKINGMODE, GSF::APPROACHLOCKINGMODE));
			ActionRegisterFutureAction(*sig, action_time + rt->approachlocking_timeout, std::unique_ptr<action>(new action_approachlockingtimeout(w, *sig)));
			return true;
		}
	}

	//now unreserve route

	GenericRouteUnreservation(rt, sig, RRF::STOP_ON_OCCUPIED_TC);

	return true;
}

bool action_reservetrack_base::GenericRouteUnreservation(const route *targrt, routingpoint *targsig, RRF extraflags) const {
	ActionRegisterFuture(std::make_shared<future_unreservetrack>(*targsig, action_time + 1, targrt, extraflags));

	auto actioncallback = [&](action &&reservation_act) {
		reservation_act.action_time++;
		reservation_act.Execute();
	};
	return targrt->PartialRouteReservationWithActions(RRF::TRYUNRESERVE | extraflags, 0, RRF::UNRESERVE | extraflags, actioncallback);
}

void action_reservetrack_base::CancelApproachLocking(genericsignal *sig) const {
	sig->SetSignalFlagsMasked(GSF::ZERO, GSF::APPROACHLOCKINGMODE);

	auto func = [&](future &f) {
		future_action_wrapper *fa = dynamic_cast<future_action_wrapper *>(&f);
		if(!fa)
			return;

		action_approachlockingtimeout *act = dynamic_cast<action_approachlockingtimeout *>(fa->act.get());

		if(act)
			ActionCancelFuture(f);
	};
	sig->EnumerateFutures(func);
}

void action_reservetrack_routeop::Deserialise(const deserialiser_input &di, error_collection &ec) {
	action_reservetrack_base::Deserialise(di, ec);
	DeserialiseRouteTargetByParentAndIndex(targetroute, di, ec, false);

	if(!targetroute)
		ec.RegisterNewError<error_deserialisation>(di, "Invalid track reservation action definition");
}

void action_reservetrack_routeop::Serialise(serialiser_output &so, error_collection &ec) const {
	action_reservetrack_base::Serialise(so, ec);
	SerialiseValueJson(targetroute->parent->GetSerialisationName(), so, "routeparent");
	SerialiseValueJson(targetroute->index, so, "routeindex");
}

void action_reservetrack_sigop::Deserialise(const deserialiser_input &di, error_collection &ec) {
	action_reservetrack_base::Deserialise(di, ec);
	std::string targetname;
	if(CheckTransJsonValue(targetname, di, "target", ec)) {
		target = FastSignalCast(w.FindTrackByName(targetname));
	}

	if(!target)
		ec.RegisterNewError<error_deserialisation>(di, "Invalid track reservation action definition");
}

void action_reservetrack_sigop::Serialise(serialiser_output &so, error_collection &ec) const {
	action_reservetrack_base::Serialise(so, ec);
	SerialiseValueJson(target->GetSerialisationName(), so, "target");
}

void action_reservetrack::ExecuteAction() const {
	if(!targetroute) return;

	TryReserveRoute(targetroute, action_time, [&](const std::shared_ptr<future> &f) {
		ActionSendReplyFuture(f);
	});
}

action_reservepath::action_reservepath(world &w_, const routingpoint *start_, const routingpoint *end_)
		: action_reservetrack_base(w_), start(start_), end(end_), allowed_route_types(route_class::AllNonOverlaps()),
				gmr_flags(GMRF::DYNPRIORITY), extraflags(RRF::IGNORE_OWN_OVERLAP) { }

action_reservepath &action_reservepath::SetGmrFlags(GMRF gmr_flags_) {
	gmr_flags = gmr_flags_;
	return *this;
}

action_reservepath &action_reservepath::SetAllowedRouteTypes(route_class::set s) {
	allowed_route_types = s;
	return *this;
}

action_reservepath &action_reservepath::SetExtraFlags(RRF extraflags_) {
	extraflags = extraflags_;
	return *this;
}

action_reservepath &action_reservepath::SetVias(via_list vias_) {
	vias = vias_;
	return *this;
}

void action_reservepath::ExecuteAction() const {
	if(!start) {
		ActionSendReplyFuture(std::make_shared<future_genericusermessage_reason>(w, action_time + 1, &w,
				"track/reservation/fail", "track/reservation/notsignal"));
		return;
	}

	std::vector<routingpoint::gmr_routeitem> routes;
	unsigned int routecount = start->GetMatchingRoutes(routes, end, allowed_route_types, gmr_flags, extraflags, vias);

	if(!routecount) {
		ActionSendReplyFuture(std::make_shared<future_genericusermessage_reason>(w, action_time + 1, &w,
				"track/reservation/fail", "track/reservation/noroute"));
		return;
	}

	std::vector<std::shared_ptr<future> > failmessages;

	for(auto it = routes.begin(); it != routes.end(); ++it) {
		if(it->rt->routecommonflags & route::RCF::EXITSIGCONTROL) {
			genericsignal *gs = FastSignalCast(it->rt->end.track, it->rt->end.direction);
			if(gs) {
				if(gs->GetCurrentForwardRoute()) {
					ActionSendReplyFuture(std::make_shared<future_genericusermessage_reason>(w, action_time + 1, &w,
							"track/reservation/fail", "track/reservation/routesetfromexitsignal"));
					return;
				}
			}
		}

		genericsignal *startgs = FastSignalCast(it->rt->start.track, it->rt->start.direction);
		bool isbackexitsigroute = false;
		startgs->EnumerateCurrentBackwardsRoutes([&](const route *r) {
			if(r->routecommonflags & route::RCF::EXITSIGCONTROL)
				isbackexitsigroute = true;
		});
		if(isbackexitsigroute) {
			ActionSendReplyFuture(std::make_shared<future_genericusermessage_reason>(w, action_time + 1, &w,
					"track/reservation/fail", "track/reservation/routesettothissignal"));
			return;
		}

		bool success = TryReserveRoute(it->rt, action_time, [&](const std::shared_ptr<future> &f) {
			if(it == routes.begin())
				failmessages.push_back(f);
		});
		if(success)
			return;
	}
	for(auto &it : failmessages) {
		ActionSendReplyFuture(it);
	}
}

void action_reservepath::Deserialise(const deserialiser_input &di, error_collection &ec) {
	action_reservetrack_base::Deserialise(di, ec);
	std::string targetname;
	if(CheckTransJsonValue(targetname, di, "start", ec)) {
		start = FastRoutingpointCast(w.FindTrackByName(targetname));
	}
	if(CheckTransJsonValue(targetname, di, "end", ec)) {
		end = FastRoutingpointCast(w.FindTrackByName(targetname));
	}
	CheckTransJsonValue(gmr_flags, di, "gmr_flags", ec);
	CheckTransJsonValue(extraflags, di, "extraflags", ec);
	route_class::DeserialiseProp("allowed_route_types", allowed_route_types, di, ec);

	auto viaparser = [&](const deserialiser_input &di, error_collection &ec) {
		routingpoint *rp = FastRoutingpointCast(w.FindTrackByName(GetType<std::string>(di.json)));
		if(rp)
			vias.push_back(rp);
	};
	vias.clear();
	CheckIterateJsonArrayOrType<std::string>(di, "vias", "via", ec, viaparser);

	if(!start)
		ec.RegisterNewError<error_deserialisation>(di, "Invalid path reservation action definition");
}

void action_reservepath::Serialise(serialiser_output &so, error_collection &ec) const {
	action_reservetrack_base::Serialise(so, ec);
	SerialiseValueJson(start->GetSerialisationName(), so, "start");
	SerialiseValueJson(end->GetSerialisationName(), so, "end");
	SerialiseValueJson(gmr_flags, so, "gmr_flags");
	SerialiseValueJson(extraflags, so, "extraflags");
	route_class::SerialiseProp("allowed_route_types", allowed_route_types, so);
	if(vias.size()) {
		so.json_out.String("vias");
		so.json_out.StartArray();
		for(auto &it : vias) {
			so.json_out.String(it->GetSerialisationName());
		}
		so.json_out.EndArray();
	}
}

void action_unreservetrack::ExecuteAction() const {
	if(target) {
		TryUnreserveRoute(target, action_time, [&](const std::shared_ptr<future> &f) {
			ActionSendReplyFuture(f);
		});
	}
}

void action_unreservetrackroute::ExecuteAction() const {
	GenericRouteUnreservation(targetroute, targetroute->start.track, RRF::STOP_ON_OCCUPIED_TC);
}

future_signalflags::future_signalflags(genericsignal &targ, world_time ft, GSF bits_, GSF mask_)
		: future(targ, ft, targ.GetWorld().MakeNewFutureID()), bits(bits_), mask(mask_) { };

void future_signalflags::ExecuteAction() {
	genericsignal *sig = dynamic_cast<genericsignal *>(&GetTarget());
	if(sig) {
		sig->SetSignalFlagsMasked(bits, mask);
	}
}

void future_signalflags::Deserialise(const deserialiser_input &di, error_collection &ec) {
	future::Deserialise(di, ec);
	CheckTransJsonValueDef(bits, di, "bits", GSF::ZERO, ec);
	CheckTransJsonValueDef(mask, di, "mask", GSF::ZERO, ec);
}

void future_signalflags::Serialise(serialiser_output &so, error_collection &ec) const {
	future::Serialise(so, ec);
	SerialiseValueJson(bits, so, "bits");
	SerialiseValueJson(mask, so, "mask");
}

void action_approachlockingtimeout::ExecuteAction() const {
	ActionRegisterFuture(std::make_shared<future_signalflags>(*target, action_time + 1, GSF::ZERO, GSF::APPROACHLOCKINGMODE));
	const route *rt = target->GetCurrentForwardRoute();
	if(rt)
		GenericRouteUnreservation(rt, target, RRF::STOP_ON_OCCUPIED_TC);
}
