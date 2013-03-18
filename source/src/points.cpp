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
#include "track.h"
#include "points.h"
#include "error.h"
#include "track_ops.h"

#include <cassert>

class train;

void genericpoints::TrainEnter(EDGETYPE direction, train *t) { }
void genericpoints::TrainLeave(EDGETYPE direction, train *t) { }


const track_target_ptr & points::GetConnectingPiece(EDGETYPE direction) const {
	if(IsOOC(0)) return empty_track_target;

	bool pointsrev = pflags & PTF::REV;

	switch(direction) {
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
	switch(direction) {
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

const track_target_ptr & points::GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) const {
	switch(direction) {
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

EDGETYPE points::GetReverseDirection(EDGETYPE direction) const {
	if(IsOOC(0)) return EDGE_NULL;

	bool pointsrev = pflags & PTF::REV;

	switch(direction) {
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

const track_target_ptr& points::GetEdgeConnectingPiece(EDGETYPE edgeid) const {
	switch(edgeid) {
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
	switch(edge) {
		case EDGE_PTS_FACE:
		case EDGE_PTS_NORMAL:
		case EDGE_PTS_REVERSE:
			return true;
		default:
			return false;
	}
}

GTF points::GetFlags(EDGETYPE direction) const {
	return GTF::ROUTEFORK | trs.GetGTReservationFlags(direction);
}

genericpoints::PTF points::GetPointFlags(unsigned int points_index) const {
	if(points_index != 0) return PTF::INVALID;
	return pflags;
}

genericpoints::PTF points::SetPointFlagsMasked(unsigned int points_index, genericpoints::PTF set_flags, genericpoints::PTF mask_flags) {
	if(points_index != 0) return PTF::INVALID;
	pflags = (pflags & (~mask_flags)) | set_flags;
	return pflags;
}

bool IsPointsRoutingDirAndIndexRev(EDGETYPE direction, unsigned int index) {
	switch(direction) {
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

bool points::Reservation(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute) {
	PTF pflags = GetPointFlags(0);
	bool rev = pflags & PTF::REV;
	if(pflags & (PTF::LOCKED | PTF::REMINDER)) {
		if(IsPointsRoutingDirAndIndexRev(direction, index) != rev) return false;
	}
	return trs.Reservation(direction, index, rr_flags, resroute);
}

void points::ReservationActions(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute, std::function<void(action &&reservation_act)> submitaction) {
	if(rr_flags & (RRF::RESERVE | RRF::DUMMY_RESERVE)) {
		bool rev = pflags & PTF::REV;
		bool newrev = IsPointsRoutingDirAndIndexRev(direction, index);
		if(newrev != rev) {
			submitaction(action_pointsaction(GetWorld(), *this, 0, newrev ? PTF::REV : PTF::ZERO, PTF::REV));
		}
	}
}

EDGETYPE points::GetAvailableAutoConnectionDirection(bool forwardconnection) const {
	if(forwardconnection && !normal.IsValid()) return EDGE_PTS_NORMAL;
	if(!forwardconnection && !prev.IsValid()) return EDGE_PTS_FACE;
	return EDGE_NULL;
}

void points::GetListOfEdges(std::vector<edgelistitem> &outputlist) const {
	outputlist.insert(outputlist.end(), { edgelistitem(EDGE_PTS_FACE, prev), edgelistitem(EDGE_PTS_NORMAL, normal), edgelistitem(EDGE_PTS_REVERSE, reverse) });
}

const track_target_ptr & catchpoints::GetConnectingPiece(EDGETYPE direction) const {
	if(IsOOC(0)) return empty_track_target;
	if(pflags & PTF::REV) return empty_track_target;

	switch(direction) {
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
	switch(direction) {
		case EDGE_FRONT:
			return 1;
		case EDGE_BACK:
			return 1;
		default:
			assert(false);
			return 0;
	}
}

const track_target_ptr & catchpoints::GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) const {
	switch(direction) {
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
	if(IsOOC(0)) return EDGE_NULL;
	if(pflags & PTF::REV) return EDGE_NULL;

	switch(direction) {
		case EDGE_FRONT:
			return EDGE_BACK;
		case EDGE_BACK:
			return EDGE_FRONT;
		default:
			assert(false);
			return EDGE_NULL;
	}
}

const track_target_ptr& catchpoints::GetEdgeConnectingPiece(EDGETYPE edgeid) const {
	switch(edgeid) {
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
	switch(edge) {
		case EDGE_FRONT:
		case EDGE_BACK:
			return true;
		default:
			return false;
	}
}

GTF catchpoints::GetFlags(EDGETYPE direction) const {
	return GTF::ROUTEFORK | trs.GetGTReservationFlags(direction);
}

genericpoints::PTF catchpoints::GetPointFlags(unsigned int points_index) const {
	if(points_index != 0) return PTF::INVALID;
	return pflags;
}

genericpoints::PTF catchpoints::SetPointFlagsMasked(unsigned int points_index, genericpoints::PTF set_flags, genericpoints::PTF mask_flags) {
	if(points_index != 0) return PTF::INVALID;
	pflags = (pflags & (~mask_flags)) | set_flags;
	return pflags;
}

bool catchpoints::Reservation(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute) {
	genericpoints::PTF pflags = GetPointFlags(0);
	if(pflags & PTF::LOCKED && pflags & PTF::REV)  return false;
	return trs.Reservation(direction, index, rr_flags, resroute);
}

void catchpoints::ReservationActions(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute, std::function<void(action &&reservation_act)> submitaction) {
	if(rr_flags & RRF::RESERVE && trs.GetReservationCount() == 0) {
		submitaction(action_pointsaction(GetWorld(), *this, 0, PTF::ZERO, PTF::REV));
	}
	else if(rr_flags & RRF::UNRESERVE && trs.GetReservationCount() == 1) {
		submitaction(action_pointsaction(GetWorld(), *this, 0, PTF::REV, PTF::REV));
	}
}

EDGETYPE catchpoints::GetAvailableAutoConnectionDirection(bool forwardconnection) const {
	if(forwardconnection && !next.IsValid()) return EDGE_BACK;
	if(!forwardconnection && !prev.IsValid()) return EDGE_FRONT;
	return EDGE_NULL;
}

void catchpoints::GetListOfEdges(std::vector<edgelistitem> &outputlist) const {
	outputlist.insert(outputlist.end(), { edgelistitem(EDGE_BACK, next), edgelistitem(EDGE_FRONT, prev) });
}

const track_target_ptr & springpoints::GetConnectingPiece(EDGETYPE direction) const {
	switch(direction) {
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

unsigned int springpoints::GetMaxConnectingPieces(EDGETYPE direction) const {
	return 1;
}

EDGETYPE springpoints::GetReverseDirection(EDGETYPE direction) const {
	switch(direction) {
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

const track_target_ptr& springpoints::GetEdgeConnectingPiece(EDGETYPE edgeid) const {
	switch(edgeid) {
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

bool springpoints::IsEdgeValid(EDGETYPE edge) const {
	switch(edge) {
		case EDGE_PTS_FACE:
		case EDGE_PTS_NORMAL:
		case EDGE_PTS_REVERSE:
			return true;
		default:
			return false;
	}
}

GTF springpoints::GetFlags(EDGETYPE direction) const {
	return GTF::ROUTEFORK | trs.GetGTReservationFlags(direction);
}

bool springpoints::Reservation(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute) {
	return trs.Reservation(direction, index, rr_flags, resroute);
}

EDGETYPE springpoints::GetAvailableAutoConnectionDirection(bool forwardconnection) const {
	if(forwardconnection && !normal.IsValid()) return EDGE_PTS_NORMAL;
	if(!forwardconnection && !prev.IsValid()) return EDGE_PTS_FACE;
	return EDGE_NULL;
}

void springpoints::GetListOfEdges(std::vector<edgelistitem> &outputlist) const {
	outputlist.insert(outputlist.end(), { edgelistitem(EDGE_PTS_FACE, prev), edgelistitem(EDGE_PTS_NORMAL, normal), edgelistitem(EDGE_PTS_REVERSE, reverse) });
}

const track_target_ptr & doubleslip::GetConnectingPiece(EDGETYPE direction) const {
	EDGETYPE exitdirection = GetReverseDirection(direction);

	if(exitdirection != EDGE_NULL) return GetInputPieceOrEmpty(exitdirection);
	else return empty_track_target;
}

unsigned int doubleslip::GetMaxConnectingPieces(EDGETYPE direction) const {
	switch(direction) {
		case EDGE_DS_FL:
			return 2 - __builtin_popcount(dsflags&(DSF::NO_FL_BL | DSF::NO_FL_BR));
		case EDGE_DS_FR:
			return 2 - __builtin_popcount(dsflags&(DSF::NO_FR_BL | DSF::NO_FR_BR));
		case EDGE_DS_BR:
			return 2 - __builtin_popcount(dsflags&(DSF::NO_FL_BR | DSF::NO_FR_BR));
		case EDGE_DS_BL:
			return 2 - __builtin_popcount(dsflags&(DSF::NO_FL_BL | DSF::NO_FR_BL));
		default:
			assert(false);
			return 0;
	}
}

const track_target_ptr & doubleslip::GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) const {
	genericpoints::PTF pf = GetCurrentPointFlags(direction);
	bool isrev = (pf & PTF::FIXED) ? (pf & PTF::REV) : index;

	EDGETYPE exitdirection = GetConnectingPointDirection(direction, isrev);
	return GetInputPieceOrEmpty(exitdirection);
}

EDGETYPE doubleslip::GetReverseDirection(EDGETYPE direction) const {
	genericpoints::PTF pf = GetCurrentPointFlags(direction);
	if(IsFlagsOOC(pf)) return EDGE_NULL;

	EDGETYPE exitdirection = GetConnectingPointDirection(direction, pf & PTF::REV);
	genericpoints::PTF exitpf = GetCurrentPointFlags(exitdirection);
	if(IsFlagsOOC(exitpf)) return EDGE_NULL;

	if(GetConnectingPointDirection(exitdirection, exitpf & PTF::REV) == direction) {
		return exitdirection;
	}
	else return EDGE_NULL;
}

const track_target_ptr& doubleslip::GetEdgeConnectingPiece(EDGETYPE edgeid) const {
	const track_target_ptr *ttp = GetInputPiece(edgeid);
	if(ttp) return *ttp;
	else {
		assert(false);
		return empty_track_target;
	}
}

bool doubleslip::IsEdgeValid(EDGETYPE edge) const {
	switch(edge) {
			case EDGE_DS_FL:
			case EDGE_DS_FR:
			case EDGE_DS_BR:
			case EDGE_DS_BL:
			return true;
		default:
			return false;
	}
}

GTF doubleslip::GetFlags(EDGETYPE direction) const {
	return GTF::ROUTEFORK | trs.GetGTReservationFlags(direction);
}

genericpoints::PTF doubleslip::GetPointFlags(unsigned int points_index) const {
	if(points_index < 4) return pflags[points_index];
	else {
		assert(false);
		return PTF::INVALID;
	}
}

genericpoints::PTF doubleslip::SetPointFlagsMasked(unsigned int points_index, genericpoints::PTF set_flags, genericpoints::PTF mask_flags) {
	if(points_index < 4) {
		auto safe_set = [&](genericpoints::PTF &flagsvar, genericpoints::PTF newflags) {
			if(!(flagsvar & PTF::FIXED)) flagsvar = (newflags & PTF::SERIALISABLE) | (flagsvar & ~PTF::SERIALISABLE);
		};

		if(pflags[points_index] & PTF::FIXED) return pflags[points_index];

		genericpoints::PTF newpointsflags = (pflags[points_index] & (~mask_flags)) | set_flags;
		if(dof == 1) {
			safe_set(pflags[0], newpointsflags);
			safe_set(pflags[1], newpointsflags);
			safe_set(pflags[2], newpointsflags);
			safe_set(pflags[3], newpointsflags);
		}
		else if(dof == 2) {
			safe_set(pflags[points_index], newpointsflags);
			safe_set(pflags[points_index ^ 1], newpointsflags ^ PTF::REV);
		}
		else if(dof == 4) safe_set(pflags[points_index], newpointsflags);
		return pflags[points_index];
	}
	else {
		return PTF::INVALID;
	}
}

bool doubleslip::Reservation(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute) {
	PTF pf = GetCurrentPointFlags(direction);
	if(pf & PTF::LOCKED) {
		if(!(pf & PTF::REV) != !index) return false;	//points locked in wrong direction
	}

	bool isrev = (pf & PTF::FIXED) ? (pf & PTF::REV) : index;
	EDGETYPE exitdirection = GetConnectingPointDirection(direction, isrev);
	PTF exitpf = GetCurrentPointFlags(exitdirection);
	bool exitpointsrev = (GetConnectingPointDirection(exitdirection, true) == direction);

	if(exitpf & PTF::LOCKED) {
		if(!(exitpf & PTF::REV) != !exitpointsrev) return false;	//points locked in wrong direction
	}

	return trs.Reservation(direction, index, rr_flags, resroute);
}

void doubleslip::ReservationActions(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute, std::function<void(action &&reservation_act)> submitaction) {
	if(rr_flags & (RRF::RESERVE | RRF::DUMMY_RESERVE)) {
		unsigned int entranceindex = GetCurrentPointIndex(direction);
		PTF entrancepf = GetCurrentPointFlags(direction);
		bool isentrancerev = (entrancepf & PTF::FIXED) ? (entrancepf & PTF::REV) : index;

		EDGETYPE exitdirection = GetConnectingPointDirection(direction, isentrancerev);

		unsigned int exitindex = GetCurrentPointIndex(exitdirection);
		PTF exitpf = GetCurrentPointFlags(exitdirection);

		bool isexitrev = (GetConnectingPointDirection(exitdirection, true) == direction);

		if(!(entrancepf & PTF::REV) != !(isentrancerev)) submitaction(action_pointsaction(GetWorld(), *this, entranceindex, isentrancerev ? PTF::REV : PTF::ZERO, PTF::REV));
		if(!(exitpf & PTF::REV) != !(isexitrev)) submitaction(action_pointsaction(GetWorld(), *this, exitindex, isexitrev ? PTF::REV : PTF::ZERO, PTF::REV));
	}
}

void doubleslip::GetListOfEdges(std::vector<edgelistitem> &outputlist) const {
	outputlist.insert(outputlist.end(), { edgelistitem(EDGE_DS_FL, frontleft), edgelistitem(EDGE_DS_FR, frontright), edgelistitem(EDGE_DS_BR, backright), edgelistitem(EDGE_DS_BL, backleft) });
}

void doubleslip::UpdatePointsFixedStatesFromMissingTrackEdges() {
	auto fixpoints = [&](EDGETYPE direction1, bool reverse1, EDGETYPE direction2, bool reverse2) {
		auto fixpoints2 = [&](EDGETYPE direction, bool reverse) {
			GetCurrentPointFlags(direction) |= PTF::FIXED;
			GetCurrentPointFlags(direction) &= ~ PTF::REV;
			GetCurrentPointFlags(direction) |= reverse ? PTF::REV : PTF::ZERO;
		};
		fixpoints2(direction1, reverse1);
		fixpoints2(direction2, reverse2);
	};

	switch(static_cast<DSF>(dsflags&DSF::NO_TRACK_MASK)) {
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
