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

	bool pointsrev = pflags & PTF_REV;

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

	bool pointsrev = pflags & PTF_REV;

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

bool points::HalfConnect(EDGETYPE this_entrance_direction, const track_target_ptr &target_entrance) {
	switch(this_entrance_direction) {
		case EDGE_PTS_FACE:
			return TryConnectPiece(prev, target_entrance);
		case EDGE_PTS_NORMAL:
			return TryConnectPiece(normal, target_entrance);
		case EDGE_PTS_REVERSE:
			return TryConnectPiece(reverse, target_entrance);
		default:
			return false;
	}
}

unsigned int points::GetFlags(EDGETYPE direction) const {
	return GTF_ROUTEFORK | trs.GetGTReservationFlags(direction);
}

unsigned int points::GetPointFlags(unsigned int points_index) const {
	if(points_index != 0) return PTF_INVALID;
	return pflags;
}

unsigned int points::SetPointFlagsMasked(unsigned int points_index, unsigned int set_flags, unsigned int mask_flags) {
	if(points_index != 0) return PTF_INVALID;
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

bool points::Reservation(EDGETYPE direction, unsigned int index, unsigned int rr_flags, const route *resroute) {
	unsigned int pflags = GetPointFlags(0);
	bool rev = pflags & PTF_REV;
	if(pflags & (PTF_LOCKED | PTF_REMINDER)) {
		if(IsPointsRoutingDirAndIndexRev(direction, index) != rev) return false;
	}
	return trs.Reservation(direction, index, rr_flags, resroute);
}

void points::ReservationActions(EDGETYPE direction, unsigned int index, unsigned int rr_flags, const route *resroute, std::function<void(action &&reservation_act)> submitaction) {
	if(rr_flags & (RRF_RESERVE | RRF_DUMMY_RESERVE)) {
		bool rev = pflags & PTF_REV;
		bool newrev = IsPointsRoutingDirAndIndexRev(direction, index);
		if(newrev != rev) {
			submitaction(action_pointsaction(GetWorld(), *this, 0, newrev ? PTF_REV : 0, PTF_REV));
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
	if(pflags & PTF_REV) return empty_track_target;

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
	if(pflags & PTF_REV) return EDGE_NULL;

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

bool catchpoints::HalfConnect(EDGETYPE this_entrance_direction, const track_target_ptr &target_entrance) {
	switch(this_entrance_direction) {
		case EDGE_FRONT:
			return TryConnectPiece(prev, target_entrance);
		case EDGE_BACK:
			return TryConnectPiece(next, target_entrance);
		default:
			return false;
	}
}

unsigned int catchpoints::GetFlags(EDGETYPE direction) const {
	return GTF_ROUTEFORK | trs.GetGTReservationFlags(direction);
}

unsigned int catchpoints::GetPointFlags(unsigned int points_index) const {
	if(points_index != 0) return PTF_INVALID;
	return pflags;
}

unsigned int catchpoints::SetPointFlagsMasked(unsigned int points_index, unsigned int set_flags, unsigned int mask_flags) {
	if(points_index != 0) return PTF_INVALID;
	pflags = (pflags & (~mask_flags)) | set_flags;
	return pflags;
}

bool catchpoints::Reservation(EDGETYPE direction, unsigned int index, unsigned int rr_flags, const route *resroute) {
	unsigned int pflags = GetPointFlags(0);
	if(pflags & PTF_LOCKED && pflags & PTF_REV)  return false;
	return trs.Reservation(direction, index, rr_flags, resroute);
}

void catchpoints::ReservationActions(EDGETYPE direction, unsigned int index, unsigned int rr_flags, const route *resroute, std::function<void(action &&reservation_act)> submitaction) {
	if(rr_flags & RRF_RESERVE && trs.GetReservationCount() == 0) {
		submitaction(action_pointsaction(GetWorld(), *this, 0, 0, PTF_REV));
	}
	else if(rr_flags & RRF_UNRESERVE && trs.GetReservationCount() == 1) {
		submitaction(action_pointsaction(GetWorld(), *this, 0, PTF_REV, PTF_REV));
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

bool springpoints::HalfConnect(EDGETYPE this_entrance_direction, const track_target_ptr &target_entrance) {
	switch(this_entrance_direction) {
		case EDGE_PTS_FACE:
			return TryConnectPiece(prev, target_entrance);
		case EDGE_PTS_NORMAL:
			return TryConnectPiece(normal, target_entrance);
		case EDGE_PTS_REVERSE:
			return TryConnectPiece(reverse, target_entrance);
		default:
			return false;
	}
}

unsigned int springpoints::GetFlags(EDGETYPE direction) const {
	return GTF_ROUTEFORK | trs.GetGTReservationFlags(direction);
}

bool springpoints::Reservation(EDGETYPE direction, unsigned int index, unsigned int rr_flags, const route *resroute) {
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
			return 2 - __builtin_popcount(dsflags&(DSF_NO_FL_BL | DSF_NO_FL_BR));
		case EDGE_DS_FR:
			return 2 - __builtin_popcount(dsflags&(DSF_NO_FR_BL | DSF_NO_FR_BR));
		case EDGE_DS_BR:
			return 2 - __builtin_popcount(dsflags&(DSF_NO_FL_BR | DSF_NO_FR_BR));
		case EDGE_DS_BL:
			return 2 - __builtin_popcount(dsflags&(DSF_NO_FL_BL | DSF_NO_FR_BL));
		default:
			assert(false);
			return 0;
	}
}

const track_target_ptr & doubleslip::GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) const {
	unsigned int pf = GetCurrentPointFlags(direction);
	bool isrev = (pf & PTF_FIXED) ? (pf & PTF_REV) : index;

	EDGETYPE exitdirection = GetConnectingPointDirection(direction, isrev);
	return GetInputPieceOrEmpty(exitdirection);
}

EDGETYPE doubleslip::GetReverseDirection(EDGETYPE direction) const {
	unsigned int pf = GetCurrentPointFlags(direction);
	if(IsFlagsOOC(pf)) return EDGE_NULL;

	EDGETYPE exitdirection = GetConnectingPointDirection(direction, pf & PTF_REV);
	unsigned int exitpf = GetCurrentPointFlags(exitdirection);
	if(IsFlagsOOC(exitpf)) return EDGE_NULL;

	if(GetConnectingPointDirection(exitdirection, exitpf & PTF_REV) == direction) {
		return exitdirection;
	}
	else return EDGE_NULL;
}

bool doubleslip::HalfConnect(EDGETYPE this_entrance_direction, const track_target_ptr &target_entrance) {
	track_target_ptr *piece = GetInputPiece(this_entrance_direction);
	if(piece) return TryConnectPiece(*piece, target_entrance);
	else return false;
}

unsigned int doubleslip::GetFlags(EDGETYPE direction) const {
	return GTF_ROUTEFORK | trs.GetGTReservationFlags(direction);
}

unsigned int doubleslip::GetPointFlags(unsigned int points_index) const {
	if(points_index < 4) return pflags[points_index];
	else {
		assert(false);
		return PTF_INVALID;
	}
}

unsigned int doubleslip::SetPointFlagsMasked(unsigned int points_index, unsigned int set_flags, unsigned int mask_flags) {
	if(points_index < 4) {
		auto safe_set = [&](unsigned int &flagsvar, unsigned int newflags) {
			if(!(flagsvar & PTF_FIXED)) flagsvar = (newflags & PTF_SERIALISABLE) | (flagsvar & ~PTF_SERIALISABLE);
		};

		if(pflags[points_index] & PTF_FIXED) return pflags[points_index];

		unsigned int newpointsflags = (pflags[points_index] & (~mask_flags)) | set_flags;
		if(dof == 1) {
			safe_set(pflags[0], newpointsflags);
			safe_set(pflags[1], newpointsflags);
			safe_set(pflags[2], newpointsflags);
			safe_set(pflags[3], newpointsflags);
		}
		else if(dof == 2) {
			safe_set(pflags[points_index], newpointsflags);
			safe_set(pflags[points_index ^ 1], newpointsflags ^ PTF_REV);
		}
		else if(dof == 4) safe_set(pflags[points_index], newpointsflags);
		return pflags[points_index];
	}
	else {
		return PTF_INVALID;
	}
}

bool doubleslip::Reservation(EDGETYPE direction, unsigned int index, unsigned int rr_flags, const route *resroute) {
	unsigned int pf = GetCurrentPointFlags(direction);
	if(pf & PTF_LOCKED) {
		if(!(pf & PTF_REV) != !index) return false;	//points locked in wrong direction
	}

	bool isrev = (pf & PTF_FIXED) ? (pf & PTF_REV) : index;
	EDGETYPE exitdirection = GetConnectingPointDirection(direction, isrev);
	unsigned int exitpf = GetCurrentPointFlags(exitdirection);
	bool exitpointsrev = (GetConnectingPointDirection(exitdirection, true) == direction);

	if(exitpf & PTF_LOCKED) {
		if(!(exitpf & PTF_REV) != !exitpointsrev) return false;	//points locked in wrong direction
	}

	return trs.Reservation(direction, index, rr_flags, resroute);
}

void doubleslip::ReservationActions(EDGETYPE direction, unsigned int index, unsigned int rr_flags, const route *resroute, std::function<void(action &&reservation_act)> submitaction) {
	if(rr_flags & (RRF_RESERVE | RRF_DUMMY_RESERVE)) {
		unsigned int entranceindex = GetCurrentPointIndex(direction);
		unsigned int entrancepf = GetCurrentPointFlags(direction);
		bool isentrancerev = (entrancepf & PTF_FIXED) ? (entrancepf & PTF_REV) : index;

		EDGETYPE exitdirection = GetConnectingPointDirection(direction, isentrancerev);

		unsigned int exitindex = GetCurrentPointIndex(exitdirection);
		unsigned int exitpf = GetCurrentPointFlags(exitdirection);

		bool isexitrev = (GetConnectingPointDirection(exitdirection, true) == direction);

		if(!(entrancepf & PTF_REV) != !(isentrancerev)) submitaction(action_pointsaction(GetWorld(), *this, entranceindex, isentrancerev ? PTF_REV : 0, PTF_REV));
		if(!(exitpf & PTF_REV) != !(isexitrev)) submitaction(action_pointsaction(GetWorld(), *this, exitindex, isexitrev ? PTF_REV : 0, PTF_REV));
	}
}

void doubleslip::GetListOfEdges(std::vector<edgelistitem> &outputlist) const {
	outputlist.insert(outputlist.end(), { edgelistitem(EDGE_DS_FL, frontleft), edgelistitem(EDGE_DS_FR, frontright), edgelistitem(EDGE_DS_BR, backright), edgelistitem(EDGE_DS_BL, backleft) });
}

void doubleslip::UpdatePointsFixedStatesFromMissingTrackEdges() {
	auto fixpoints = [&](EDGETYPE direction1, bool reverse1, EDGETYPE direction2, bool reverse2) {
		auto fixpoints2 = [&](EDGETYPE direction, bool reverse) {
			GetCurrentPointFlags(direction) |= PTF_FIXED;
			GetCurrentPointFlags(direction) &= ~ PTF_REV;
			GetCurrentPointFlags(direction) |= reverse ? PTF_REV : 0;
		};
		fixpoints2(direction1, reverse1);
		fixpoints2(direction2, reverse2);
	};

	switch(dsflags&DSF_NO_TRACK_MASK) {
		case DSF_NO_FL_BL:
			fixpoints(EDGE_DS_FL, false, EDGE_DS_BL, false);
			break;
		case DSF_NO_FR_BL:
			fixpoints(EDGE_DS_FR, true, EDGE_DS_BL, true);
			break;
		case DSF_NO_FL_BR:
			fixpoints(EDGE_DS_FL, true, EDGE_DS_BR, true);
			break;
		case DSF_NO_FR_BR:
			fixpoints(EDGE_DS_FR, false, EDGE_DS_BR, false);
			break;
	}
}
