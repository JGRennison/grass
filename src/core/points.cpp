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
#include "util/error.h"
#include "util/util.h"
#include "core/track.h"
#include "core/points.h"
#include "core/track_ops.h"
#include "core/param.h"

#include <cassert>

class train;

void generic_points::TrainEnter(EDGETYPE direction, train *t) { }
void generic_points::TrainLeave(EDGETYPE direction, train *t) { }

GTF generic_points::GetFlags(EDGETYPE direction) const {
	return GTF::ROUTE_FORK | trs.GetGTReservationFlags(direction);
}

generic_points::PTF generic_points::SetPointsFlagsMasked(unsigned int points_index, generic_points::PTF set_flags, generic_points::PTF mask_flags) {
	PTF &pflags = GetPointsFlagsRef(points_index);

	if (pflags & PTF::FIXED || pflags & PTF::INVALID) {
		return pflags;
	}

	PTF old_pflags = pflags;
	pflags = (pflags & (~mask_flags)) | (set_flags & mask_flags);
	if (old_pflags != pflags) {
		MarkUpdated();
	}

	std::vector<points_coupling> *couplings = GetCouplingVector(points_index);
	if (couplings) {
		for (auto &it : *couplings) {
			PTF curmask = mask_flags;
			PTF curbits = set_flags;
			if (it.xormask & PTF::REV) {
				curmask = swap_single_bits(curmask, PTF::FAILED_NORM, PTF::FAILED_REV);
				curbits = swap_single_bits(curbits, PTF::FAILED_NORM, PTF::FAILED_REV);
			}
			curbits ^= it.xormask;
			PTF old_cp_pflags = *(it.pflags);
			*(it.pflags) = (*(it.pflags) & (~curmask)) | (curbits & curmask);
			if (old_cp_pflags != *(it.pflags)) {
				it.targ->MarkUpdated();
			}
		}
	}
	return pflags;
}

bool generic_points::ShouldAutoNormalise(unsigned int index, generic_points::PTF change_flags) const {
	if (trs.GetReservationCount()) {
		return false;
	}

	PTF new_flags = GetPointsFlags(index) ^ change_flags;
	if (IsFlagsImmovable(new_flags)) {
		return false;
	}
	if (new_flags & PTF::OOC) {
		return false;
	}
	return (new_flags & PTF::REV);
}

void generic_points::CommonReservationAction(unsigned int points_index, EDGETYPE direction, unsigned int index,
		RRF rr_flags, const route *res_route, std::function<void(action &&reservation_act)> submit_action) {
	PTF pflags = GetPointsFlags(points_index);
	if (pflags & PTF::AUTO_NORMALISE && rr_flags & RRF::UNRESERVE && trs.GetReservationCount() == 1 && !IsFlagsImmovable(pflags)) {
		submit_action(action_points_auto_normalise(GetWorld(), *this, points_index));
	}
}

generic_track::edge_track_target points::GetConnectingPiece(EDGETYPE direction) {
	if (IsOOC(0)) {
		return empty_track_target;
	}

	bool pointsrev = pflags & PTF::REV;

	switch (direction) {
		case EDGE_PTS_FACE:
			return pointsrev ? reverse : normal;

		case EDGE_PTS_NORMAL:
			return pointsrev ? empty_track_target : prev;

		case EDGE_PTS_REVERSE:
			return pointsrev ? prev : empty_track_target;

		default:
			assert(false);
			return empty_track_target;
	}
}

unsigned int points::GetMaxConnectingPieces(EDGETYPE direction) const {
	switch (direction) {
		case EDGE_PTS_FACE:
			return 2;

		case EDGE_PTS_NORMAL:
			return 1;

		case EDGE_PTS_REVERSE:
			return 1;

		default:
			assert(false);
			return 0;
	}
}

generic_track::edge_track_target points::GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) {
	switch (direction) {
		case EDGE_PTS_FACE:
			return index == 1 ? reverse : normal;

		case EDGE_PTS_NORMAL:
			return prev;

		case EDGE_PTS_REVERSE:
			return prev;

		default:
			assert(false);
			return empty_track_target;
	}
}

unsigned int points::GetCurrentNominalConnectionIndex(EDGETYPE direction) const {
	if (direction == EDGE_PTS_FACE && pflags & PTF::REV) {
		return 1;
	} else {
		return 0;
	}
}

EDGETYPE points::GetReverseDirection(EDGETYPE direction) const {
	if (IsOOC(0)) {
		return EDGE_NULL;
	}

	bool pointsrev = pflags & PTF::REV;

	switch (direction) {
		case EDGE_PTS_FACE:
			return pointsrev ? EDGE_PTS_REVERSE : EDGE_PTS_NORMAL;

		case EDGE_PTS_NORMAL:
			return pointsrev ? EDGE_NULL : EDGE_PTS_FACE;

		case EDGE_PTS_REVERSE:
			return pointsrev ? EDGE_PTS_FACE : EDGE_NULL;

		default:
			assert(false);
			return EDGE_NULL;
	}
}

generic_track::edge_track_target points::GetEdgeConnectingPiece(EDGETYPE edgeid) {
	switch (edgeid) {
		case EDGE_PTS_FACE:
			return prev;

		case EDGE_PTS_NORMAL:
			return normal;

		case EDGE_PTS_REVERSE:
			return reverse;

		default:
			assert(false);
			return empty_track_target;
	}
}

bool points::IsEdgeValid(EDGETYPE edge) const {
	switch (edge) {
		case EDGE_PTS_FACE:
		case EDGE_PTS_NORMAL:
		case EDGE_PTS_REVERSE:
			return true;

		default:
			return false;
	}
}

const generic_points::PTF &points::GetPointsFlagsRef(unsigned int points_index) const {
	if (points_index != 0) {
		return fail_pflags;
	}
	return pflags;
}

void GetReservationFailureReason(generic_points::PTF pflags, std::string *fail_reason_key) {
	if (fail_reason_key && pflags & generic_points::PTF::LOCKED) {
		*fail_reason_key = "points/locked";
	} else if (fail_reason_key && pflags & generic_points::PTF::REMINDER) {
		*fail_reason_key = "points/reminderset";
	}
}

bool IsPointsRoutingDirAndIndexRev(EDGETYPE direction, unsigned int index) {
	switch (direction) {
		case EDGE_PTS_FACE:
			return (index == 1);

		case EDGE_PTS_NORMAL:
			return false;

		case EDGE_PTS_REVERSE:
			return true;

		default:
			return false;
	}
}

bool points::ReservationV(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *res_route, std::string* fail_reason_key) {
	PTF pflags = GetPointsFlags(0);
	bool rev = pflags & PTF::REV;
	if (IsFlagsImmovable(pflags)) {
		if (IsPointsRoutingDirAndIndexRev(direction, index) != rev) {
			GetReservationFailureReason(pflags, fail_reason_key);
			return false;
		}
	}
	return trs.Reservation(direction, index, rr_flags, res_route, fail_reason_key);
}

void points::ReservationActionsV(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *res_route,
		std::function<void(action &&reservation_act)> submit_action) {
	if (rr_flags & RRF::RESERVE) {
		bool rev = pflags & PTF::REV;
		bool newrev = IsPointsRoutingDirAndIndexRev(direction, index);
		if (newrev != rev) {
			submit_action(action_points_action(GetWorld(), *this, 0, newrev ? PTF::REV : PTF::ZERO, PTF::REV,
					action_points_action::APAF::NO_OVERLAP_SWING | action_points_action::APAF::NO_POINTS_NORMALISE));
		}
	}
	CommonReservationAction(0, direction, index, rr_flags, res_route, submit_action);
}

EDGETYPE points::GetAvailableAutoConnectionDirection(bool forward_connection) const {
	if (forward_connection && !normal.IsValid()) {
		return EDGE_PTS_NORMAL;
	}
	if (!forward_connection && !prev.IsValid()) {
		return EDGE_PTS_FACE;
	}
	return EDGE_NULL;
}

void points::GetListOfEdges(std::vector<edgelistitem> &output_list) const {
	output_list.insert(output_list.end(), { edgelistitem(EDGE_PTS_FACE, prev), edgelistitem(EDGE_PTS_NORMAL, normal), edgelistitem(EDGE_PTS_REVERSE, reverse) });
}

bool points::IsCoupleable(EDGETYPE direction) const {
	return (direction == EDGE_PTS_NORMAL || direction == EDGE_PTS_REVERSE) && !(pflags & PTF::COUPLED);
}

void points::GetCouplingPointsFlagsByEdge(EDGETYPE direction, std::vector<points_coupling> &output) {
	output.emplace_back(&pflags, (direction == EDGE_PTS_REVERSE)?PTF::REV:PTF::ZERO, this, 0);
}

void points::CouplePointsFlagsAtIndexTo(unsigned int index, const points_coupling &pc) {
	couplings.push_back(pc);
}

void points::InitSightingDistances() {
	sighting_distances.emplace_back(EDGE_PTS_FACE, SIGHTING_DISTANCE_POINTS);
	sighting_distances.emplace_back(EDGE_PTS_NORMAL, SIGHTING_DISTANCE_POINTS);
	sighting_distances.emplace_back(EDGE_PTS_REVERSE, SIGHTING_DISTANCE_POINTS);
}

generic_track::edge_track_target catchpoints::GetConnectingPiece(EDGETYPE direction) {
	if (IsOOC(0)) {
		return empty_track_target;
	}
	if (!(pflags & PTF::REV)) {
		return empty_track_target;
	}

	switch (direction) {
		case EDGE_FRONT:
			return next;

		case EDGE_BACK:
			return prev;

		default:
			assert(false);
			return empty_track_target;
	}
}

unsigned int catchpoints::GetMaxConnectingPieces(EDGETYPE direction) const {
	switch (direction) {
		case EDGE_FRONT:
			return 1;

		case EDGE_BACK:
			return 1;

		default:
			assert(false);
			return 0;
	}
}

generic_track::edge_track_target catchpoints::GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) {
	switch (direction) {
		case EDGE_FRONT:
			return next;

		case EDGE_BACK:
			return prev;

		default:
			assert(false);
			return empty_track_target;
	}
}

EDGETYPE catchpoints::GetReverseDirection(EDGETYPE direction) const {
	if (IsOOC(0)) {
		return EDGE_NULL;
	}
	if (!(pflags & PTF::REV)) {
		return EDGE_NULL;
	}

	switch (direction) {
		case EDGE_FRONT:
			return EDGE_BACK;

		case EDGE_BACK:
			return EDGE_FRONT;

		default:
			assert(false);
			return EDGE_NULL;
	}
}

generic_track::edge_track_target catchpoints::GetEdgeConnectingPiece(EDGETYPE edgeid) {
	switch (edgeid) {
		case EDGE_FRONT:
			return prev;

		case EDGE_BACK:
			return next;

		default:
			assert(false);
			return empty_track_target;
	}
}

bool catchpoints::IsEdgeValid(EDGETYPE edge) const {
	switch (edge) {
		case EDGE_FRONT:
		case EDGE_BACK:
			return true;

		default:
			return false;
	}
}

const generic_points::PTF &catchpoints::GetPointsFlagsRef(unsigned int points_index) const {
	if (points_index != 0) {
		return fail_pflags;
	}
	return pflags;
}

bool catchpoints::ReservationV(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *res_route, std::string* fail_reason_key) {
	generic_points::PTF pflags = GetPointsFlags(0);
	if (IsFlagsImmovable(pflags) && !(pflags & PTF::REV)) {
		GetReservationFailureReason(pflags, fail_reason_key);
		return false;
	}
	return trs.Reservation(direction, index, rr_flags, res_route, fail_reason_key);
}

void catchpoints::ReservationActionsV(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *res_route, std::function<void(action &&reservation_act)> submit_action) {
	if (rr_flags & RRF::RESERVE && trs.GetReservationCount() == 0) {
		submit_action(action_points_action(GetWorld(), *this, 0, PTF::REV, PTF::REV,
				action_points_action::APAF::NO_OVERLAP_SWING | action_points_action::APAF::NO_POINTS_NORMALISE));
	}
	CommonReservationAction(0, direction, index, rr_flags, res_route, submit_action);
}

EDGETYPE catchpoints::GetAvailableAutoConnectionDirection(bool forward_connection) const {
	if (forward_connection && !next.IsValid()) {
		return EDGE_BACK;
	}
	if (!forward_connection && !prev.IsValid()) {
		return EDGE_FRONT;
	}
	return EDGE_NULL;
}

void catchpoints::GetListOfEdges(std::vector<edgelistitem> &output_list) const {
	output_list.insert(output_list.end(), { edgelistitem(EDGE_BACK, next), edgelistitem(EDGE_FRONT, prev) });
}

void catchpoints::InitSightingDistances() {
	sighting_distances.emplace_back(EDGE_BACK, SIGHTING_DISTANCE_POINTS);
	sighting_distances.emplace_back(EDGE_FRONT, SIGHTING_DISTANCE_POINTS);
}

generic_track::edge_track_target spring_points::GetConnectingPiece(EDGETYPE direction) {
	switch (direction) {
		case EDGE_PTS_FACE:
			return sendreverse ? reverse : normal;

		case EDGE_PTS_NORMAL:
			return prev;

		case EDGE_PTS_REVERSE:
			return prev;

		default:
			assert(false);
			return empty_track_target;
	}
}

unsigned int spring_points::GetMaxConnectingPieces(EDGETYPE direction) const {
	return 1;
}

EDGETYPE spring_points::GetReverseDirection(EDGETYPE direction) const {
	switch (direction) {
		case EDGE_PTS_FACE:
			return sendreverse ? EDGE_PTS_REVERSE : EDGE_PTS_NORMAL;

		case EDGE_PTS_NORMAL:
			return EDGE_PTS_FACE;

		case EDGE_PTS_REVERSE:
			return EDGE_PTS_FACE;

		default:
			assert(false);
			return EDGE_NULL;
	}
}

generic_track::edge_track_target spring_points::GetEdgeConnectingPiece(EDGETYPE edgeid) {
	switch (edgeid) {
		case EDGE_PTS_FACE:
			return prev;

		case EDGE_PTS_NORMAL:
			return normal;

		case EDGE_PTS_REVERSE:
			return reverse;

		default:
			assert(false);
			return empty_track_target;
	}
}

bool spring_points::IsEdgeValid(EDGETYPE edge) const {
	switch (edge) {
		case EDGE_PTS_FACE:
		case EDGE_PTS_NORMAL:
		case EDGE_PTS_REVERSE:
			return true;

		default:
			return false;
	}
}

GTF spring_points::GetFlags(EDGETYPE direction) const {
	return GTF::ROUTE_FORK | trs.GetGTReservationFlags(direction);
}

bool spring_points::ReservationV(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *res_route, std::string* fail_reason_key) {
	return trs.Reservation(direction, index, rr_flags, res_route);
}

EDGETYPE spring_points::GetAvailableAutoConnectionDirection(bool forward_connection) const {
	if (forward_connection && !normal.IsValid()) {
		return EDGE_PTS_NORMAL;
	}
	if (!forward_connection && !prev.IsValid()) {
		return EDGE_PTS_FACE;
	}
	return EDGE_NULL;
}

void spring_points::GetListOfEdges(std::vector<edgelistitem> &output_list) const {
	output_list.insert(output_list.end(), { edgelistitem(EDGE_PTS_FACE, prev), edgelistitem(EDGE_PTS_NORMAL, normal), edgelistitem(EDGE_PTS_REVERSE, reverse) });
}

generic_track::edge_track_target double_slip::GetConnectingPiece(EDGETYPE direction) {
	EDGETYPE exitdirection = GetReverseDirection(direction);

	if (exitdirection != EDGE_NULL) {
		return GetInputPieceOrEmpty(exitdirection);
	} else {
		return empty_track_target;
	}
}

unsigned int double_slip::GetMaxConnectingPieces(EDGETYPE direction) const {
	switch (direction) {
		case EDGE_DS_FL:
			return 2 - __builtin_popcount(flag_unwrap<DSF>(dsflags & (DSF::NO_FL_BL | DSF::NO_FL_BR)));

		case EDGE_DS_FR:
			return 2 - __builtin_popcount(flag_unwrap<DSF>(dsflags & (DSF::NO_FR_BL | DSF::NO_FR_BR)));

		case EDGE_DS_BR:
			return 2 - __builtin_popcount(flag_unwrap<DSF>(dsflags & (DSF::NO_FL_BR | DSF::NO_FR_BR)));

		case EDGE_DS_BL:
			return 2 - __builtin_popcount(flag_unwrap<DSF>(dsflags & (DSF::NO_FL_BL | DSF::NO_FR_BL)));

		default:
			assert(false);
			return 0;
	}
}

generic_track::edge_track_target double_slip::GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) {
	generic_points::PTF pf = GetCurrentPointFlags(direction);
	bool isrev = (pf & PTF::FIXED) ? static_cast<bool>(pf & PTF::REV) : index != 0;

	EDGETYPE exitdirection = GetConnectingPointDirection(direction, isrev);
	return GetInputPieceOrEmpty(exitdirection);
}

unsigned int double_slip::GetCurrentNominalConnectionIndex(EDGETYPE direction) const {
	generic_points::PTF pf = GetCurrentPointFlags(direction);
	if (!(pf & PTF::FIXED) && pf & PTF::REV) {
		return 1;
	} else {
		return 0;
	}
}

EDGETYPE double_slip::GetReverseDirection(EDGETYPE direction) const {
	generic_points::PTF pf = GetCurrentPointFlags(direction);
	if (IsFlagsOOC(pf)) {
		return EDGE_NULL;
	}

	EDGETYPE exitdirection = GetConnectingPointDirection(direction, pf & PTF::REV);
	generic_points::PTF exitpf = GetCurrentPointFlags(exitdirection);
	if (IsFlagsOOC(exitpf)) {
		return EDGE_NULL;
	}

	if (GetConnectingPointDirection(exitdirection, exitpf & PTF::REV) == direction) {
		return exitdirection;
	} else {
		return EDGE_NULL;
	}
}

generic_track::edge_track_target double_slip::GetEdgeConnectingPiece(EDGETYPE edgeid) {
	track_target_ptr *ttp = GetInputPiece(edgeid);
	if (ttp) {
		return *ttp;
	} else {
		assert(false);
		return empty_track_target;
	}
}

bool double_slip::IsEdgeValid(EDGETYPE edge) const {
	switch (edge) {
			case EDGE_DS_FL:
			case EDGE_DS_FR:
			case EDGE_DS_BR:
			case EDGE_DS_BL:
			return true;

		default:
			return false;
	}
}

const generic_points::PTF &double_slip::GetPointsFlagsRef(unsigned int points_index) const {
	if (points_index < 4) {
		return pflags[points_index];
	} else {
		assert(false);
		return fail_pflags;
	}
}

bool double_slip::ReservationV(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *res_route, std::string* fail_reason_key) {
	PTF pf = GetCurrentPointFlags(direction);
	if (IsFlagsImmovable(pf)) {
		if (!(pf & PTF::REV) != !index) {
			GetReservationFailureReason(pf, fail_reason_key);
			return false;    //points locked in wrong direction
		}
	}

	bool isrev = (pf & PTF::FIXED) ? static_cast<bool>(pf & PTF::REV) : index != 0;
	EDGETYPE exitdirection = GetConnectingPointDirection(direction, isrev);
	PTF exitpf = GetCurrentPointFlags(exitdirection);
	bool exitpointsrev = (GetConnectingPointDirection(exitdirection, true) == direction);

	if (IsFlagsImmovable(exitpf)) {
		if (!(exitpf & PTF::REV) != !exitpointsrev) {
			GetReservationFailureReason(exitpf, fail_reason_key);
			return false;    //points locked in wrong direction
		}
	}

	return trs.Reservation(direction, index, rr_flags, res_route, fail_reason_key);
}

void double_slip::ReservationActionsV(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *res_route, std::function<void(action &&reservation_act)> submit_action) {
	unsigned int entranceindex = GetPointsIndexByEdge(direction);
	PTF entrancepf = GetCurrentPointFlags(direction);
	bool isentrancerev = (entrancepf & PTF::FIXED) ? static_cast<bool>(entrancepf & PTF::REV) : index != 0;

	EDGETYPE exitdirection = GetConnectingPointDirection(direction, isentrancerev);
	unsigned int exitindex = GetPointsIndexByEdge(exitdirection);
	PTF exitpf = GetCurrentPointFlags(exitdirection);

	if (rr_flags & RRF::RESERVE) {
		bool isexitrev = (GetConnectingPointDirection(exitdirection, true) == direction);

		if (!(entrancepf & PTF::REV) != !(isentrancerev)) {
			submit_action(action_points_action(GetWorld(), *this, entranceindex, isentrancerev ? PTF::REV : PTF::ZERO, PTF::REV,
					action_points_action::APAF::NO_OVERLAP_SWING | action_points_action::APAF::NO_POINTS_NORMALISE));
		}
		if (!(exitpf & PTF::REV) != !(isexitrev)) {
			submit_action(action_points_action(GetWorld(), *this, exitindex, isexitrev ? PTF::REV : PTF::ZERO, PTF::REV,
					action_points_action::APAF::NO_OVERLAP_SWING | action_points_action::APAF::NO_POINTS_NORMALISE));
		}
	}

	CommonReservationAction(entranceindex, direction, index, rr_flags, res_route, submit_action);
	CommonReservationAction(exitindex, direction, index, rr_flags, res_route, submit_action);
}

void double_slip::GetListOfEdges(std::vector<edgelistitem> &output_list) const {
	output_list.insert(output_list.end(), { edgelistitem(EDGE_DS_FL, frontleft), edgelistitem(EDGE_DS_FR, frontright),
			edgelistitem(EDGE_DS_BR, backright), edgelistitem(EDGE_DS_BL, backleft) });
}

void double_slip::UpdatePointsFixedStatesFromMissingTrackEdges() {
	auto fixpoints = [&](EDGETYPE direction1, bool reverse1, EDGETYPE direction2, bool reverse2) {
		auto fixpoints2 = [&](EDGETYPE direction, bool reverse) {
			GetCurrentPointFlags(direction) |= PTF::FIXED;
			GetCurrentPointFlags(direction) &= ~ PTF::REV;
			GetCurrentPointFlags(direction) |= reverse ? PTF::REV : PTF::ZERO;
		};
		fixpoints2(direction1, reverse1);
		fixpoints2(direction2, reverse2);
	};

	switch (static_cast<DSF>(dsflags&DSF::NO_TRACK_MASK)) {
		case DSF::NO_FL_BL:
			fixpoints(EDGE_DS_FL, false, EDGE_DS_BL, false);
			break;

		case DSF::NO_FR_BL:
			fixpoints(EDGE_DS_FR, true, EDGE_DS_BL, true);
			break;

		case DSF::NO_FL_BR:
			fixpoints(EDGE_DS_FL, true, EDGE_DS_BR, true);
			break;

		case DSF::NO_FR_BR:
			fixpoints(EDGE_DS_FR, false, EDGE_DS_BR, false);
			break;

		default:
			break;
	}
}

void double_slip::UpdateInternalCoupling() {
	for (unsigned int i = 0; i < 4; i++) {
		couplings[i].clear();
		if (pflags[i] & PTF::FIXED) continue;

		auto couple_with = [&](unsigned int index, PTF xormask) {
			if (!(pflags[index] & PTF::FIXED)) {
				couplings[i].emplace_back(&pflags[index], xormask, this, index);
			}
		};

		if (dof == 1) {
			couple_with(i ^ 1, PTF::ZERO);
			couple_with(i ^ 2, PTF::ZERO);
			couple_with(i ^ 3, PTF::ZERO);
		} else if (dof == 2) {
			couple_with(i ^ 1, PTF::REV);
		}
		SetPointsFlagsMasked(i, pflags[i]&PTF::SERIALISABLE, PTF::SERIALISABLE);
	}
}

bool double_slip::IsCoupleable(EDGETYPE direction) const {
	if (dof != 2) return false;
	return !(GetCurrentPointFlags(GetConnectingPointDirection(direction, true)) & PTF::COUPLED ||
			GetCurrentPointFlags(GetConnectingPointDirection(direction, false)) & PTF::COUPLED);
}

void double_slip::GetCouplingPointsFlagsByEdge(EDGETYPE direction, std::vector<points_coupling> &output) {
	auto docoupling = [&](bool reverse) {
		EDGETYPE oppositeedge = GetConnectingPointDirection(direction, reverse);
		if (GetCurrentPointFlags(oppositeedge) & PTF::FIXED) {
			return;
		}
		bool revforaccess = (GetConnectingPointDirection(oppositeedge, true) == direction);
		output.emplace_back(&GetCurrentPointFlags(oppositeedge), revforaccess ? PTF::REV : PTF::ZERO, this, GetPointsIndexByEdge(oppositeedge));
	};
	docoupling(true);
	docoupling(false);
}

void double_slip::CouplePointsFlagsAtIndexTo(unsigned int index, const points_coupling &pc) {
	couplings[index].push_back(pc);
}

void double_slip::InitSightingDistances() {
	sighting_distances.emplace_back(EDGE_DS_FL, SIGHTING_DISTANCE_POINTS);
	sighting_distances.emplace_back(EDGE_DS_FR, SIGHTING_DISTANCE_POINTS);
	sighting_distances.emplace_back(EDGE_DS_BL, SIGHTING_DISTANCE_POINTS);
	sighting_distances.emplace_back(EDGE_DS_BR, SIGHTING_DISTANCE_POINTS);
}
