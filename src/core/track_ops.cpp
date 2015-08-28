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
#include "core/track_reservation.h"
#include "core/track_ops.h"
#include "core/track.h"
#include "core/points.h"
#include "core/serialisable_impl.h"
#include "core/signal.h"
#include "core/text_pool.h"
#include "core/track_circuit.h"
#include "core/route_types_serialisation.h"


void future_points_action::ExecuteAction() {
	generic_points *gp = dynamic_cast<generic_points *>(&GetTarget());
	if (gp) {
		gp->SetPointsFlagsMasked(index, bits, mask);
	}
}

void future_points_action::Deserialise(const deserialiser_input &di, error_collection &ec) {
	future::Deserialise(di, ec);
	CheckTransJsonValueDef(index, di, "index", 0, ec);
	CheckTransJsonValueDef(bits, di, "bits", generic_points::PTF::ZERO, ec);
	CheckTransJsonValueDef(mask, di, "mask", generic_points::PTF::ZERO, ec);
}

void future_points_action::Serialise(serialiser_output &so, error_collection &ec) const {
	future::Serialise(so, ec);
	SerialiseValueJson(index, so, "index");
	SerialiseValueJson(bits, so, "bits");
	SerialiseValueJson(mask, so, "mask");
}

// Generate flag change for a nominal normalise and reverse operation
action_points_action::action_points_action(world &w_, generic_points &targ, unsigned int index_, bool reverse)
		: action(w_), target(&targ), index(index_) {
	generic_points::PTF pflags = target->GetPointsFlags(index);
	if (static_cast<bool>(pflags & generic_points::PTF::REV) != reverse) {
		if (pflags & generic_points::PTF::LOCKED) {
			// unlock
			bits = generic_points::PTF::ZERO;
			mask = generic_points::PTF::LOCKED;
		} else {
			// Change position
			bits = SetOrClearBits<generic_points::PTF>(generic_points::PTF::ZERO, generic_points::PTF::REV, reverse);
			mask = generic_points::PTF::REV;
		}
	} else {
		// Set locked state
		bits = generic_points::PTF::LOCKED;
		mask = generic_points::PTF::LOCKED;
	}
}

void action_points_action::ExecuteAction() const {
	if (!target) return;

	generic_points::PTF old_pflags = target->GetPointsFlags(index);
	generic_points::PTF change_flags = (old_pflags ^ bits) & mask;

	generic_points::PTF immediate_action_bits = generic_points::PTF::ZERO;
	generic_points::PTF immediate_action_mask = generic_points::PTF::ZERO;

	std::function<void()> overlap_callback;

	if (change_flags & generic_points::PTF::REV) {
		if (old_pflags & generic_points::PTF::LOCKED) {
			ActionSendReplyFuture(std::make_shared<future_generic_user_message_reason>(*target, action_time+1, &w,
					"track_ops/pointsunmovable", "points/locked"));
			return;
		}
		if (old_pflags & generic_points::PTF::REMINDER) {
			ActionSendReplyFuture(std::make_shared<future_generic_user_message_reason>(*target, action_time+1, &w,
					"track_ops/pointsunmovable", "points/reminderset"));
			return;
		}
		if (!(aflags & APAF::IGNORE_RESERVATION) && target->GetFlags(target->GetDefaultValidDirecton()) & GTF::ROUTE_SET) {
			std::string failreason = "track/reserved";
			if (!TrySwingOverlap(overlap_callback, !!(bits & generic_points::PTF::REV), &failreason)) {
				ActionSendReplyFuture(std::make_shared<future_generic_user_message_reason>(*target, action_time+1, &w,
						"track_ops/pointsunmovable", failreason));
				return;
			}
		}

		CancelFutures(index, generic_points::PTF::ZERO, generic_points::PTF::OOC);
		immediate_action_bits |= (bits & generic_points::PTF::REV) | generic_points::PTF::OOC;
		immediate_action_mask |= generic_points::PTF::REV | generic_points::PTF::OOC;

		//code for random failures goes here

		ActionRegisterFuture(std::make_shared<future_points_action>(*target, GetPointsMovementCompletionTime(), index,
				generic_points::PTF::ZERO, generic_points::PTF::OOC));
	}
	generic_points::PTF immediate_change_flags = change_flags & (generic_points::PTF::LOCKED | generic_points::PTF::REMINDER);
	immediate_action_mask |= immediate_change_flags;
	immediate_action_bits |= bits & immediate_change_flags;

	ActionRegisterFuture(std::make_shared<future_points_action>(*target, action_time + 1, index, immediate_action_bits, immediate_action_mask));

	if (overlap_callback) {
		overlap_callback();
	}

	if (old_pflags & generic_points::PTF::AUTO_NORMALISE) {
		if (!(aflags & APAF::NO_POINTS_NORMALISE) && target->ShouldAutoNormalise(index, change_flags)) {
			w.SubmitAction(action_points_auto_normalise(w, *target, index));
		} else {
			action_points_auto_normalise::CancelFutures(target, index);
		}
	}
}

world_time action_points_action::GetPointsMovementCompletionTime() const {
	return action_time + 5000;
}

void action_points_action::CancelFutures(unsigned int index, generic_points::PTF setmask, generic_points::PTF clearmask) const {

	auto check_instance = [&](generic_points *p, unsigned int index, generic_points::PTF setmask, generic_points::PTF clearmask) {
		auto func = [&](future &f) {
			future_points_action *fp = dynamic_cast<future_points_action *>(&f);
			if (!fp) {
				return;
			}
			if (fp->index != index) {
				return;
			}
			generic_points::PTF foundset = fp->mask & fp->bits & setmask;
			generic_points::PTF foundunset = fp->mask & (~fp->bits) & clearmask;
			if (foundset || foundunset) {
				flagwrapper<generic_points::PTF> newmask = fp->mask & ~(foundset | foundunset);
				if (newmask) {
					ActionRegisterFuture(std::make_shared<future_points_action>(*target, fp->GetTriggerTime(), index, fp->bits, newmask));
				}
				ActionCancelFuture(f);
			}
		};

		p->EnumerateFutures(func);
	};
	check_instance(target, index, setmask, clearmask);
	std::vector<generic_points::points_coupling> *coupling = target->GetCouplingVector(index);
	if (coupling) {
		for (auto &it : *coupling) {
			generic_points::PTF rev = (setmask ^ clearmask) & it.xormask;
			check_instance(it.targ, it.index, setmask ^ rev, clearmask ^ rev);
		}
	}
}

bool action_points_action::TrySwingOverlap(std::function<void()> &overlap_callback, bool setting_reverse, std::string *fail_reason_key) const {
	if (aflags & APAF::NO_OVERLAP_SWING) return false;

	const route *foundoverlap = nullptr;
	bool failed = false;
	target->ReservationEnumeration([&](const route *reserved_route, EDGE direction, unsigned int index, RRF rr_flags) {
		if (!route_class::IsOverlap(reserved_route->type)) {
			failed = true;
			return;
		}
		if (foundoverlap) {
			failed = true;
			return;
		}
		foundoverlap = reserved_route;
	}, RRF::RESERVE | RRF::PROVISIONAL_RESERVE);
	if (failed || !foundoverlap) {
		return false;
	}

	generic_signal *start = FastSignalCast(foundoverlap->start.track, foundoverlap->start.direction);
	if (!start) {
		return false;
	}
	if (!start->IsOverlapSwingPermitted(fail_reason_key)) {
		return false;
	}

	//temporarily "move" points
	generic_points::PTF saved_pflags = target->SetPointsFlagsMasked(index, setting_reverse ? generic_points::PTF::REV : generic_points::PTF::ZERO,
			generic_points::PTF::REV);

	const route *best_new_overlap = nullptr;
	int best_overlap_score = INT_MIN;
	start->EnumerateAvailableOverlaps([&](const route *rt, int score) {
		if (score > best_overlap_score) {
			best_new_overlap = rt;
			best_overlap_score = score;
		}
	}, RRF::IGNORE_OWN_OVERLAP);

	//move back
	target->SetPointsFlagsMasked(index, saved_pflags, ~generic_points::PTF::ZERO);

	bool result = false;

	if (best_new_overlap && best_new_overlap != foundoverlap) {
		overlap_callback = [=]() {
			auto action_callback = [&](action &&reservation_act) {
				reservation_act.Execute();
			};
			if (foundoverlap) {
				foundoverlap->RouteReservationActions(RRF::UNRESERVE, action_callback);
				ActionRegisterFuture(std::make_shared<future_unreserve_track>(*start, action_time + 1, foundoverlap));
			}
			if (best_new_overlap) {
				best_new_overlap->RouteReservationActions(RRF::RESERVE, action_callback);
				ActionRegisterFuture(std::make_shared<future_reserve_track>(*start, action_time + 1, best_new_overlap));
				best_new_overlap->RouteReservation(RRF::PROVISIONAL_RESERVE | RRF::IGNORE_OWN_OVERLAP);
			}
		};
		result = true;
	}

	return result;
}

void action_points_action::Deserialise(const deserialiser_input &di, error_collection &ec) {
	action::Deserialise(di, ec);

	std::string targetname;
	if (CheckTransJsonValue(targetname, di, "target", ec)) {
		target = w.FindTrackByNameCast<generic_points>(targetname);
	}

	if (!target) {
		ec.RegisterNewError<error_deserialisation>(di, "Invalid action_points_action definition");
	}

	CheckTransJsonValueDef(index, di, "index", 0, ec);
	CheckTransJsonValueDef(bits, di, "bits", generic_points::PTF::ZERO, ec);
	CheckTransJsonValueDef(mask, di, "mask", generic_points::PTF::ZERO, ec);
	CheckTransJsonValueDef(aflags, di, "aflags", APAF::ZERO, ec);
}

void action_points_action::Serialise(serialiser_output &so, error_collection &ec) const {
	action::Serialise(so, ec);
	SerialiseValueJson(target->GetSerialisationName(), so, "target");
	SerialiseValueJson(index, so, "index");
	SerialiseValueJson(bits, so, "bits");
	SerialiseValueJson(mask, so, "mask");
	SerialiseValueJson(aflags, so, "aflags");
}

void action_points_auto_normalise::ExecuteAction() const {
	if (!target) {
		return;
	}

	if (HasFutures(target, index)) {
		return;
	}

	ActionRegisterFutureAction(*target, GetNormalisationStartTime(),
			std::unique_ptr<action>(new action_points_action(w, *target, index, generic_points::PTF::ZERO, generic_points::PTF::REV, action_points_action::APAF::IS_POINTS_NORMALISE)));
}

void action_points_auto_normalise::Deserialise(const deserialiser_input &di, error_collection &ec) {
	action::Deserialise(di, ec);

	std::string targetname;
	if (CheckTransJsonValue(targetname, di, "target", ec)) {
		target = w.FindTrackByNameCast<generic_points>(targetname);
	}

	if (!target) {
		ec.RegisterNewError<error_deserialisation>(di, "Invalid action_points_auto_normalise definition");
	}

	CheckTransJsonValueDef(index, di, "index", 0, ec);
}

void action_points_auto_normalise::Serialise(serialiser_output &so, error_collection &ec) const {
	action::Serialise(so, ec);
	SerialiseValueJson(target->GetSerialisationName(), so, "target");
	SerialiseValueJson(index, so, "index");
}

world_time action_points_auto_normalise::GetNormalisationStartTime() const {
	return action_time + 2000;
}

bool action_points_auto_normalise::HandleFuturesGeneric(generic_points *target, unsigned int index, bool cancel) {
	bool found = false;
	target->EnumerateFutures([&](future &f) {
		future_action_wrapper *faw = dynamic_cast<future_action_wrapper *>(&f);
		if (!faw) {
			return;
		}
		action_points_action *apan = dynamic_cast<action_points_action *>(faw->act.get());
		if (!apan) {
			return;
		}
		if (!(apan->GetFlags() & action_points_action::APAF::IS_POINTS_NORMALISE)) {
			return;
		}
		if (apan->GetIndex() != index) {
			return;
		}

		found = true;
		if (cancel) {
			apan->ActionCancelFuture(f);
		}
	});
	return found;
}

void action_points_auto_normalise::CancelFutures(generic_points *target, unsigned int index) {
	HandleFuturesGeneric(target, index, true);
}

bool action_points_auto_normalise::HasFutures(generic_points *target, unsigned int index) {
	return HandleFuturesGeneric(target, index, false);
}

void future_route_operation_base::Deserialise(const deserialiser_input &di, error_collection &ec) {
	future::Deserialise(di, ec);
	DeserialiseRouteTargetByParentAndIndex(reserved_route, di, ec, false);
}

void future_route_operation_base::Serialise(serialiser_output &so, error_collection &ec) const {
	future::Serialise(so, ec);
	SerialiseRouteTargetByParentAndIndex(reserved_route, so, ec);
}

void future_reserve_track_base::ExecuteAction() {
	if (reserved_route) {
		reserved_route->RouteReservation(rflags);
	}
}

void future_reserve_track_base::Deserialise(const deserialiser_input &di, error_collection &ec) {
	future_route_operation_base::Deserialise(di, ec);
	CheckTransJsonValueDef(rflags, di, "rflags", RRF::ZERO, ec);
}

void future_reserve_track_base::Serialise(serialiser_output &so, error_collection &ec) const {
	future_route_operation_base::Serialise(so, ec);
	SerialiseValueJson(rflags, so, "rflags");
}

const route *action_reserve_track_base::TestSwingOverlapAndReserve(const route *target_route, std::string *fail_reason_key) const {
	generic_signal *gs = FastSignalCast(target_route->start.track, target_route->start.direction);
	if (!gs) {
		return nullptr;
	}
	if (!target_route->RouteReservation(RRF::TRY_RESERVE | RRF::IGNORE_OWN_OVERLAP)) {
		return nullptr;    //can't reserve even when ignoring the overlap
	}

	if (!gs->IsOverlapSwingPermitted(fail_reason_key)) {
		return nullptr;
	}

	std::vector<std::pair<int, const route *> > candidates;
	gs->EnumerateAvailableOverlaps([&](const route *rt, int score) {
		candidates.push_back(std::make_pair(-score, rt));
	}, RRF::IGNORE_OWN_OVERLAP);
	std::sort(candidates.begin(), candidates.end());

	const route *result = nullptr;
	for (auto &it : candidates) {
		if (target_route->IsRouteSubSet(it.second)) {
			result = it.second;
			break;
		}
	}
	return result;
}

//return true on success
bool action_reserve_track_base::TryReserveRoute(const route *rt, world_time action_time,
		std::function<void(const std::shared_ptr<future> &f)> error_handler) const {
	//disallow if non-overlap route already set from start point in given direction
	//but silently accept if the set route is identical to the one trying to be set
	bool found_route = false;
	bool route_conflict = false;
	rt->start.track->ReservationEnumerationInDirection(rt->start.direction,
			[&](const route *reserved_route, EDGE direction, unsigned int index, RRF rr_flags) {
		if (route_class::IsOverlap(reserved_route->type)) {
			return;
		}
		if (!(rr_flags & RRF::START_PIECE)) {
			return;
		}
		found_route = true;
		if (reserved_route != rt) {
			route_conflict = true;
		}
	}, RRF::RESERVE | RRF::PROVISIONAL_RESERVE);
	if (found_route) {
		if (route_conflict) {
			error_handler(std::make_shared<future_generic_user_message_reason>(w, action_time + 1, &w,
					"track/reservation/fail", "track/reservation/alreadyset"));
			return false;
		} else {
			generic_signal *sig = FastSignalCast(rt->start.track, rt->start.direction);
			if (sig) {
				CancelApproachLocking(sig);
			}
			return true;
		}
	}

	const route *swing_this_overlap = nullptr;
	const route *remove_prev_overlap = nullptr;
	std::string tryreservation_fail_reason_key = "generic/failurereason";
	bool success = rt->RouteReservation(RRF::TRY_RESERVE, &tryreservation_fail_reason_key);
	if (!success) {
		swing_this_overlap = TestSwingOverlapAndReserve(rt, &tryreservation_fail_reason_key);
		if (swing_this_overlap) {
			success = true;
			generic_signal *sig = FastSignalCast(rt->start.track, rt->start.direction);
			if (sig) {
				remove_prev_overlap = sig->GetCurrentForwardOverlap();
			}
		}
	}

	if (!success) {
		error_handler(std::make_shared<future_generic_user_message_reason>(w, action_time + 1, &w,
				"track/reservation/fail", tryreservation_fail_reason_key));
		return false;
	}

	const route *best_overlap = nullptr;
	if (rt->overlap_type != route_class::ID::NONE) {
		//need an overlap too
		best_overlap = rt->end.track->FindBestOverlap(route_class::Flag(rt->overlap_type));
		if (!best_overlap) {
			error_handler(std::make_shared<future_generic_user_message_reason>(w, action_time + 1, &w,
					"track/reservation/fail", "track/reservation/overlap/noneavailable"));
			return false;
		}
	}

	//route is OK, now reserve it

	auto action_callback = [&](action &&reservation_act) {
		action_points_action *apa = dynamic_cast<action_points_action*>(&reservation_act);
		if (apa) {
			apa->SetFlagsMasked(action_points_action::APAF::IGNORE_RESERVATION, action_points_action::APAF::IGNORE_RESERVATION);
		}
		reservation_act.Execute();
	};

	if (remove_prev_overlap) {
		remove_prev_overlap->RouteReservationActions(RRF::UNRESERVE, action_callback);
		ActionRegisterFuture(std::make_shared<future_unreserve_track>(*rt->start.track, action_time + 1, remove_prev_overlap));
	}
	if (swing_this_overlap) {
		swing_this_overlap->RouteReservationActions(RRF::RESERVE, action_callback);
		ActionRegisterFuture(std::make_shared<future_reserve_track>(*rt->start.track, action_time + 1, swing_this_overlap));
		swing_this_overlap->RouteReservation(RRF::PROVISIONAL_RESERVE | RRF::IGNORE_OWN_OVERLAP);
	}
	rt->RouteReservationActions(RRF::RESERVE, action_callback);
	ActionRegisterFuture(std::make_shared<future_reserve_track>(*rt->start.track, action_time + 1, rt));
	rt->RouteReservation(RRF::PROVISIONAL_RESERVE);
	if (best_overlap) {
		best_overlap->RouteReservationActions(RRF::RESERVE, action_callback);
		ActionRegisterFuture(std::make_shared<future_reserve_track>(*rt->start.track, action_time + 1, best_overlap));
		best_overlap->RouteReservation(RRF::PROVISIONAL_RESERVE);
	}
	return true;
}

//return true on success
bool action_reserve_track_base::TryUnreserveRoute(routing_point *startsig, world_time action_time,
		std::function<void(const std::shared_ptr<future> &f)> error_handler) const {
	generic_signal *sig = FastSignalCast(startsig);
	if (!sig) {
		error_handler(std::make_shared<future_generic_user_message_reason>(w, action_time + 1, &w,
				"track/unreservation/fail", "track/reservation/notsignal"));
		return false;
	}

	if (sig->GetSignalFlags() & GSF::AUTO_SIGNAL) {
		error_handler(std::make_shared<future_generic_user_message_reason>(w, action_time + 1, &w,
				"track/unreservation/fail", "track/unreservation/autosignal"));
		return false;
	}

	const route *rt = sig->GetCurrentForwardRoute();
	if (!rt) {
		error_handler(std::make_shared<future_generic_user_message_reason>(w, action_time + 1, &w,
				"track/unreservation/fail", "track/notreserved"));
		return false;
	}

	if (sig->GetSignalFlags() & GSF::APPROACH_LOCKING_MODE)
		return true;    //don't try to unreserve again

	//approach locking checks
	if (sig->GetAspect() > 0) {
		bool approachlocking_engage = false;
		unsigned int backaspect = 0;

		auto checksignal = [&](const generic_signal *gs) -> bool {
			bool aspectchange = gs->GetReservedAspect() > backaspect;
				//cancelling route would reduce signal aspect
				//this includes existing approach control aspects

			backaspect++;
			return aspectchange;
		};

		auto check_piece = [&](const track_target_ptr &piece) -> bool {
			track_circuit *tc = piece.track->GetTrackCircuit();
			if (tc && tc->Occupied()) {
				approachlocking_engage = true;    //found an occupied piece, train is on approach
				return false;
			} else {
				return true;
			}
		};

		sig->BackwardsReservedTrackScan(checksignal, check_piece);

		if (approachlocking_engage) {
			ActionRegisterFuture(std::make_shared<future_signal_flags>(*sig, action_time+1, GSF::APPROACH_LOCKING_MODE, GSF::APPROACH_LOCKING_MODE));
			ActionRegisterFutureAction(*sig, action_time + rt->approach_locking_timeout, std::unique_ptr<action>(new action_approach_locking_timeout(w, *sig)));
			return true;
		}
	}

	//now unreserve route

	GenericRouteUnreservation(rt, sig, RRF::STOP_ON_OCCUPIED_TC);

	return true;
}

bool action_reserve_track_base::GenericRouteUnreservation(const route *targrt, routing_point *targsig, RRF extra_flags) const {
	ActionRegisterFuture(std::make_shared<future_unreserve_track>(*targsig, action_time + 1, targrt, extra_flags));

	auto action_callback = [&](action &&reservation_act) {
		reservation_act.action_time++;
		reservation_act.Execute();
	};
	return targrt->PartialRouteReservationWithActions(RRF::TRY_UNRESERVE | extra_flags, 0, RRF::UNRESERVE | extra_flags, action_callback);
}

void action_reserve_track_base::CancelApproachLocking(generic_signal *sig) const {
	sig->SetSignalFlagsMasked(GSF::ZERO, GSF::APPROACH_LOCKING_MODE);

	auto func = [&](future &f) {
		future_action_wrapper *fa = dynamic_cast<future_action_wrapper *>(&f);
		if (!fa) {
			return;
		}

		action_approach_locking_timeout *act = dynamic_cast<action_approach_locking_timeout *>(fa->act.get());

		if (act) {
			ActionCancelFuture(f);
		}
	};
	sig->EnumerateFutures(func);
}

void action_reserve_track_routeop::Deserialise(const deserialiser_input &di, error_collection &ec) {
	action_reserve_track_base::Deserialise(di, ec);
	DeserialiseRouteTargetByParentAndIndex(targetroute, di, ec, false);

	if (!targetroute) {
		ec.RegisterNewError<error_deserialisation>(di, "Invalid track reservation action definition");
	}
}

void action_reserve_track_routeop::Serialise(serialiser_output &so, error_collection &ec) const {
	action_reserve_track_base::Serialise(so, ec);
	SerialiseRouteTargetByParentAndIndex(targetroute, so, ec);
}

void action_reserve_track_sigop::Deserialise(const deserialiser_input &di, error_collection &ec) {
	action_reserve_track_base::Deserialise(di, ec);
	std::string targetname;
	if (CheckTransJsonValue(targetname, di, "target", ec)) {
		target = FastSignalCast(w.FindTrackByName(targetname));
	}

	if (!target) {
		ec.RegisterNewError<error_deserialisation>(di, "Invalid track reservation action definition");
	}
}

void action_reserve_track_sigop::Serialise(serialiser_output &so, error_collection &ec) const {
	action_reserve_track_base::Serialise(so, ec);
	SerialiseValueJson(target->GetSerialisationName(), so, "target");
}

void action_reserve_track::ExecuteAction() const {
	if (!targetroute) return;

	TryReserveRoute(targetroute, action_time, [&](const std::shared_ptr<future> &f) {
		ActionSendReplyFuture(f);
	});
}

action_reserve_path::action_reserve_path(world &w_, const routing_point *start_, const routing_point *end_)
		: action_reserve_track_base(w_), start(start_), end(end_), allowed_route_types(route_class::AllNonOverlaps()),
				gmr_flags(GMRF::DYNAMIC_PRIORITY), extra_flags(RRF::IGNORE_OWN_OVERLAP) { }

action_reserve_path &action_reserve_path::SetGmrFlags(GMRF gmr_flags_) {
	gmr_flags = gmr_flags_;
	return *this;
}

action_reserve_path &action_reserve_path::SetAllowedRouteTypes(route_class::set s) {
	allowed_route_types = s;
	return *this;
}

action_reserve_path &action_reserve_path::SetExtraFlags(RRF extra_flags_) {
	extra_flags = extra_flags_;
	return *this;
}

action_reserve_path &action_reserve_path::SetVias(via_list vias_) {
	vias = vias_;
	return *this;
}

void action_reserve_path::ExecuteAction() const {
	if (!start) {
		ActionSendReplyFuture(std::make_shared<future_generic_user_message_reason>(w, action_time + 1, &w,
				"track/reservation/fail", "track/reservation/notsignal"));
		return;
	}

	std::vector<routing_point::gmr_route_item> routes;
	unsigned int routecount = start->GetMatchingRoutes(routes, end, allowed_route_types, gmr_flags, extra_flags, vias);

	if (!routecount) {
		ActionSendReplyFuture(std::make_shared<future_generic_user_message_reason>(w, action_time + 1, &w,
				"track/reservation/fail", "track/reservation/noroute"));
		return;
	}

	std::vector<std::shared_ptr<future> > failmessages;

	for (auto it = routes.begin(); it != routes.end(); ++it) {
		if (it->rt->route_common_flags & route::RCF::EXIT_SIGNAL_CONTROL) {
			generic_signal *gs = FastSignalCast(it->rt->end.track, it->rt->end.direction);
			if (gs) {
				if (gs->GetCurrentForwardRoute()) {
					ActionSendReplyFuture(std::make_shared<future_generic_user_message_reason>(w, action_time + 1, &w,
							"track/reservation/fail", "track/reservation/routesetfromexitsignal"));
					return;
				}
			}
		}

		generic_signal *startgs = FastSignalCast(it->rt->start.track, it->rt->start.direction);
		bool isbackexitsigroute = false;
		startgs->EnumerateCurrentBackwardsRoutes([&](const route *r) {
			if (r->route_common_flags & route::RCF::EXIT_SIGNAL_CONTROL) {
				isbackexitsigroute = true;
			}
		});
		if (isbackexitsigroute) {
			ActionSendReplyFuture(std::make_shared<future_generic_user_message_reason>(w, action_time + 1, &w,
					"track/reservation/fail", "track/reservation/routesettothissignal"));
			return;
		}

		bool success = TryReserveRoute(it->rt, action_time, [&](const std::shared_ptr<future> &f) {
			if (it == routes.begin()) {
				failmessages.push_back(f);
			}
		});
		if (success) {
			return;
		}
	}
	for (auto &it : failmessages) {
		ActionSendReplyFuture(it);
	}
}

void action_reserve_path::Deserialise(const deserialiser_input &di, error_collection &ec) {
	action_reserve_track_base::Deserialise(di, ec);
	std::string targetname;
	if (CheckTransJsonValue(targetname, di, "start", ec)) {
		start = FastRoutingpointCast(w.FindTrackByName(targetname));
	}
	if (CheckTransJsonValue(targetname, di, "end", ec)) {
		end = FastRoutingpointCast(w.FindTrackByName(targetname));
	}
	CheckTransJsonValue(gmr_flags, di, "gmr_flags", ec);
	CheckTransJsonValue(extra_flags, di, "extra_flags", ec);
	route_class::DeserialiseProp("allowed_route_types", allowed_route_types, di, ec);

	auto viaparser = [&](const deserialiser_input &di, error_collection &ec) {
		routing_point *rp = FastRoutingpointCast(w.FindTrackByName(GetType<std::string>(di.json)));
		if (rp) {
			vias.push_back(rp);
		}
	};
	vias.clear();
	CheckIterateJsonArrayOrType<std::string>(di, "vias", "via", ec, viaparser);

	if (!start) {
		ec.RegisterNewError<error_deserialisation>(di, "Invalid path reservation action definition");
	}
}

void action_reserve_path::Serialise(serialiser_output &so, error_collection &ec) const {
	action_reserve_track_base::Serialise(so, ec);
	SerialiseValueJson(start->GetSerialisationName(), so, "start");
	SerialiseValueJson(end->GetSerialisationName(), so, "end");
	SerialiseValueJson(gmr_flags, so, "gmr_flags");
	SerialiseValueJson(extra_flags, so, "extra_flags");
	route_class::SerialiseProp("allowed_route_types", allowed_route_types, so);
	if (vias.size()) {
		so.json_out.String("vias");
		so.json_out.StartArray();
		for (auto &it : vias) {
			so.json_out.String(it->GetSerialisationName());
		}
		so.json_out.EndArray();
	}
}

void action_unreserve_track::ExecuteAction() const {
	if (target) {
		TryUnreserveRoute(target, action_time, [&](const std::shared_ptr<future> &f) {
			ActionSendReplyFuture(f);
		});
	}
}

void action_unreserve_track_route::ExecuteAction() const {
	if (targetroute) {
		GenericRouteUnreservation(targetroute, targetroute->start.track, RRF::STOP_ON_OCCUPIED_TC);
	}
}

future_signal_flags::future_signal_flags(generic_signal &targ, world_time ft, GSF bits_, GSF mask_)
		: future(targ, ft, targ.GetWorld().MakeNewFutureID()), bits(bits_), mask(mask_) { };

void future_signal_flags::ExecuteAction() {
	generic_signal *sig = dynamic_cast<generic_signal *>(&GetTarget());
	if (sig) {
		sig->SetSignalFlagsMasked(bits, mask);
	}
}

void future_signal_flags::Deserialise(const deserialiser_input &di, error_collection &ec) {
	future::Deserialise(di, ec);
	CheckTransJsonValueDef(bits, di, "bits", GSF::ZERO, ec);
	CheckTransJsonValueDef(mask, di, "mask", GSF::ZERO, ec);
}

void future_signal_flags::Serialise(serialiser_output &so, error_collection &ec) const {
	future::Serialise(so, ec);
	SerialiseValueJson(bits, so, "bits");
	SerialiseValueJson(mask, so, "mask");
}

void action_approach_locking_timeout::ExecuteAction() const {
	ActionRegisterFuture(std::make_shared<future_signal_flags>(*target, action_time + 1, GSF::ZERO, GSF::APPROACH_LOCKING_MODE));
	const route *rt = target->GetCurrentForwardRoute();
	if (rt) {
		GenericRouteUnreservation(rt, target, RRF::STOP_ON_OCCUPIED_TC);
	}
}
