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
#include "train.h"
#include "error.h"
#include "track_ops.h"

#include <cassert>
#include <cstring>
#include <initializer_list>

const track_location empty_track_location;
const track_target_ptr empty_track_target;

unsigned int speedrestrictionset::GetTrackSpeedLimitByClass(const std::string &speedclass, unsigned int default_max) const {
	for(auto it = speeds.begin(); it != speeds.end(); ++it) {
		if(it->speedclass.empty() || it->speedclass == speedclass) {
			if(it->speed < default_max) default_max = it->speed;
		}
	}
	return default_max;
}

unsigned int speedrestrictionset::GetTrainTrackSpeedLimit(train *t) const {
	return GetTrackSpeedLimitByClass(t->GetVehSpeedClass(), t->GetMaxVehSpeed());
}

track_circuit *generictrack::GetTrackCircuit() const {
	return 0;
}

const speedrestrictionset *generictrack::GetSpeedRestrictions() const {
	return 0;
}

const tractionset *generictrack::GetTractionTypes() const {
	return 0;
}

generictrack & generictrack::SetLength(unsigned int length) {
	return *this;
}
generictrack & generictrack::AddSpeedRestriction(speed_restriction sr) {
	return *this;
}
generictrack & generictrack::SetElevationDelta(unsigned int elevationdelta) {
	return *this;
}
generictrack & generictrack::SetTrackCircuit(track_circuit *tc) {
	return *this;
}

int generictrack::GetPartialElevationDelta(EDGETYPE direction, unsigned int displacement) const {
	int elevationdelta = GetElevationDelta(direction);
	if(elevationdelta) {
		return (((int64_t) elevationdelta) * ((int64_t) displacement)) / ((int64_t) GetLength(direction));
	}
	else return 0;
}

bool generictrack::TryConnectPiece(track_target_ptr &piece_var, const track_target_ptr &new_target) {
	if(piece_var.IsValid() && piece_var != new_target) {
		return false;
	}
	else {
		piece_var = new_target;
		return true;
	}
}

bool generictrack::FullConnect(EDGETYPE this_entrance_direction, const track_target_ptr &target_entrance, error_collection &ec) {
	if(HalfConnect(this_entrance_direction, target_entrance) && target_entrance.track->HalfConnect(target_entrance.direction, track_target_ptr(this, this_entrance_direction))) {
		return true;
	}
	else {
		ec.RegisterError(std::unique_ptr<error_obj>(new error_trackconnection(track_target_ptr(this, this_entrance_direction), target_entrance)));
		return false;
	}
}

class error_track_unconnected : public layout_initialisation_error_obj {
	public:
	error_track_unconnected(const track_target_ptr &edge) {
		msg << "Track edge unconnected: " << edge;
	}
};

bool generictrack::AutoConnections(error_collection &ec) {
	if(prevtrack) {
		EDGETYPE prevedge = prevtrack->GetAvailableAutoConnectionDirection(!(prevtrack->gt_flags & GTF_REVERSEAUTOCONN));
		EDGETYPE thisedge = GetAvailableAutoConnectionDirection(gt_flags & GTF_REVERSEAUTOCONN);
		if(prevedge != EDGE_NULL && thisedge != EDGE_NULL) {
			if(!FullConnect(thisedge, track_target_ptr(prevtrack, prevedge), ec)) return false;
		}
	}
	return true;
}

bool generictrack::CheckUnconnectedEdges(error_collection &ec) {
	bool result = true;

	std::vector<edgelistitem> edgelist;
	GetListOfEdges(edgelist);
	for(auto it = edgelist.begin(); it != edgelist.end(); ++it) {
		if(! it->target->IsValid()) {
			ec.RegisterError(std::unique_ptr<error_obj>(new error_track_unconnected(track_target_ptr(this, it->edge))));
			result = false;
		}
	}

	return result;
}

bool generictrack::PostLayoutInit(error_collection &ec) {
	return true;
}

void trackseg::TrainEnter(EDGETYPE direction, train *t) {
	traincount++;
	track_circuit *tc = GetTrackCircuit();
	if(tc) tc->TrainEnter(t);
	const speedrestrictionset *speeds = GetSpeedRestrictions();
	if(speeds) t->AddCoveredTrackSpeedLimit(speeds->GetTrainTrackSpeedLimit(t));
}

void trackseg::TrainLeave(EDGETYPE direction, train *t) {
	traincount--;
	track_circuit *tc = GetTrackCircuit();
	if(tc) tc->TrainLeave(t);
	const speedrestrictionset *speeds = GetSpeedRestrictions();
	if(speeds) t->RemoveCoveredTrackSpeedLimit(speeds->GetTrainTrackSpeedLimit(t));
}

bool trackseg::HalfConnect(EDGETYPE this_entrance_direction, const track_target_ptr &target_entrance) {
	switch(this_entrance_direction) {
		case EDGE_FRONT:
			return TryConnectPiece(prev, target_entrance);
		case EDGE_BACK:
			return TryConnectPiece(next, target_entrance);
		default:
			return false;
	}
}

const track_target_ptr & trackseg::GetConnectingPiece(EDGETYPE direction) const {
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

unsigned int trackseg::GetMaxConnectingPieces(EDGETYPE direction) const {
	return 1;
}
const track_target_ptr & trackseg::GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) const {
	return GetConnectingPiece(direction);
}

int trackseg::GetElevationDelta(EDGETYPE direction) const {
	return (direction==EDGE_FRONT) ? elevationdelta : -elevationdelta;
}

unsigned int trackseg::GetStartOffset(EDGETYPE direction) const {
	return (direction == EDGE_BACK) ? GetLength(direction) : 0;
}

const speedrestrictionset *trackseg::GetSpeedRestrictions() const {
	return &speed_limits;
}

const tractionset *trackseg::GetTractionTypes() const {
	return &tractiontypes;
}

unsigned int trackseg::GetLength(EDGETYPE direction) const {
	return length;
}

unsigned int trackseg::GetNewOffset(EDGETYPE direction, unsigned int currentoffset, unsigned int step) const {
	switch(direction) {
		case EDGE_FRONT:
			return currentoffset + step;
		case EDGE_BACK:
			return currentoffset - step;
		default:
			assert(false);
			return currentoffset;
	}
}

unsigned int trackseg::GetRemainingLength(EDGETYPE direction, unsigned int currentoffset) const {
	switch(direction) {
		case EDGE_FRONT:
			return length - currentoffset;
		case EDGE_BACK:
			return currentoffset;
		default:
			assert(false);
			return 0;
	}
}

track_circuit *trackseg::GetTrackCircuit() const {
	return tc;
}

unsigned int trackseg::GetFlags(EDGETYPE direction) const {
	return trs.GetGTReservationFlags(direction);
}

EDGETYPE trackseg::GetReverseDirection(EDGETYPE direction) const {
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

trackseg & trackseg::SetLength(unsigned int length) {
	this->length = length;
	return *this;
}
trackseg & trackseg::AddSpeedRestriction(speed_restriction sr) {
	this->speed_limits.AddSpeedRestriction(sr);
	return *this;
}
trackseg & trackseg::SetElevationDelta(unsigned int elevationdelta) {
	this->elevationdelta = elevationdelta;
	return *this;
}
trackseg & trackseg::SetTrackCircuit(track_circuit *tc) {
	this->tc = tc;
	return *this;
}

bool trackseg::Reservation(EDGETYPE direction, unsigned int index, unsigned int rr_flags, route *resroute) {
	return trs.Reservation(direction, index, rr_flags, resroute);
}

EDGETYPE trackseg::GetAvailableAutoConnectionDirection(bool forwardconnection) const {
	if(forwardconnection && !next.IsValid()) return EDGE_BACK;
	if(!forwardconnection && !prev.IsValid()) return EDGE_FRONT;
	return EDGE_NULL;
}

void trackseg::GetListOfEdges(std::vector<edgelistitem> &outputlist) const {
	outputlist.insert(outputlist.end(), { edgelistitem(EDGE_BACK, next), edgelistitem(EDGE_FRONT, prev) });
}

unsigned int genericzlentrack::GetStartOffset(EDGETYPE direction) const {
	return 0;
}

int genericzlentrack::GetElevationDelta(EDGETYPE direction) const {
	return 0;
}

unsigned int genericzlentrack::GetNewOffset(EDGETYPE direction, unsigned int currentoffset, unsigned int step) const {
	if(currentoffset > 0) assert(false);
	return 0;
}

unsigned int genericzlentrack::GetRemainingLength(EDGETYPE direction, unsigned int currentoffset) const {
	if(currentoffset > 0) assert(false);
	return 0;
}

unsigned int genericzlentrack::GetLength(EDGETYPE direction) const {
	return 0;
}

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

bool points::Reservation(EDGETYPE direction, unsigned int index, unsigned int rr_flags, route *resroute) {
	unsigned int pflags = GetPointFlags(0);
	bool rev = pflags & PTF_REV;
	if(pflags & (PTF_LOCKED | PTF_REMINDER)) {
		if(IsPointsRoutingDirAndIndexRev(direction, index) != rev) return false;
	}
	return trs.Reservation(direction, index, rr_flags, resroute);
}

void points::ReservationActions(EDGETYPE direction, unsigned int index, unsigned int rr_flags, route *resroute, world &w, std::function<void(action &&reservation_act)> submitaction) {
	if(rr_flags & RRF_RESERVE) {
		bool rev = pflags & PTF_REV;
		bool newrev = IsPointsRoutingDirAndIndexRev(direction, index);
		if(newrev != rev) {
			submitaction(action_pointsaction(w, *this, 0, newrev ? PTF_REV : 0, PTF_REV));
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

bool catchpoints::Reservation(EDGETYPE direction, unsigned int index, unsigned int rr_flags, route *resroute) {
	unsigned int pflags = GetPointFlags(0);
	if(pflags & PTF_LOCKED && pflags & PTF_REV)  return false;
	return trs.Reservation(direction, index, rr_flags, resroute);
}

void catchpoints::ReservationActions(EDGETYPE direction, unsigned int index, unsigned int rr_flags, route *resroute, world &w, std::function<void(action &&reservation_act)> submitaction) {
	if(rr_flags & RRF_RESERVE) {
		submitaction(action_pointsaction(w, *this, 0, 0, PTF_REV));
	}
	else if(rr_flags & RRF_UNRESERVE) {
		submitaction(action_pointsaction(w, *this, 0, PTF_REV, PTF_REV));
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

bool springpoints::Reservation(EDGETYPE direction, unsigned int index, unsigned int rr_flags, route *resroute) {
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

const track_target_ptr & crossover::GetConnectingPiece(EDGETYPE direction) const {
	switch(direction) {
		case EDGE_X_N:
			return north;
		case EDGE_X_S:
			return south;
		case EDGE_X_W:
			return west;
		case EDGE_X_E:
			return east;
		default:
			assert(false);
			return empty_track_target;
	}
}

unsigned int crossover::GetMaxConnectingPieces(EDGETYPE direction) const {
	return 1;
}

EDGETYPE crossover::GetReverseDirection(EDGETYPE direction) const {
	switch(direction) {
		case EDGE_X_N:
			return EDGE_X_S;
		case EDGE_X_S:
			return EDGE_X_N;
		case EDGE_X_W:
			return EDGE_X_E;
		case EDGE_X_E:
			return EDGE_X_W;
		default:
			assert(false);
			return EDGE_NULL;
	}
}

bool crossover::HalfConnect(EDGETYPE this_entrance_direction, const track_target_ptr &target_entrance) {
	switch(this_entrance_direction) {
		case EDGE_X_N:
			return TryConnectPiece(north, target_entrance);
		case EDGE_X_S:
			return TryConnectPiece(south, target_entrance);
		case EDGE_X_W:
			return TryConnectPiece(west, target_entrance);
		case EDGE_X_E:
			return TryConnectPiece(east, target_entrance);
		default:
			return false;
	}
}

unsigned int crossover::GetFlags(EDGETYPE direction) const {
	return GTF_ROUTEFORK | trs.GetGTReservationFlags(direction);
}

bool crossover::Reservation(EDGETYPE direction, unsigned int index, unsigned int rr_flags, route *resroute) {
	return trs.Reservation(direction, index, rr_flags, resroute);
}

void crossover::GetListOfEdges(std::vector<edgelistitem> &outputlist) const {
	outputlist.insert(outputlist.end(), { edgelistitem(EDGE_X_N, north), edgelistitem(EDGE_X_S, south), edgelistitem(EDGE_X_W, west), edgelistitem(EDGE_X_E, east) });
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

bool doubleslip::Reservation(EDGETYPE direction, unsigned int index, unsigned int rr_flags, route *resroute) {
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

void doubleslip::ReservationActions(EDGETYPE direction, unsigned int index, unsigned int rr_flags, route *resroute, world &w, std::function<void(action &&reservation_act)> submitaction) {
	if(rr_flags & RRF_RESERVE) {
		unsigned int entranceindex = GetCurrentPointIndex(direction);
		unsigned int entrancepf = GetCurrentPointFlags(direction);
		bool isentrancerev = (entrancepf & PTF_FIXED) ? (entrancepf & PTF_REV) : index;

		EDGETYPE exitdirection = GetConnectingPointDirection(direction, isentrancerev);

		unsigned int exitindex = GetCurrentPointIndex(exitdirection);
		unsigned int exitpf = GetCurrentPointFlags(exitdirection);

		bool isexitrev = (GetConnectingPointDirection(exitdirection, true) == direction);

		if(!(entrancepf & PTF_REV) != !(isentrancerev)) submitaction(action_pointsaction(w, *this, entranceindex, isentrancerev ? PTF_REV : 0, PTF_REV));
		if(!(exitpf & PTF_REV) != !(isexitrev)) submitaction(action_pointsaction(w, *this, exitindex, isexitrev ? PTF_REV : 0, PTF_REV));
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

layout_initialisation_error_obj::layout_initialisation_error_obj() {
	msg << "Track layout initialisation failure: ";
}

error_trackconnection::error_trackconnection(const track_target_ptr &targ1, const track_target_ptr &targ2) {
	msg << "Track Connection Error: Could Not Connect: " << targ1 << " to " << targ2;
}

error_trackconnection_notfound::error_trackconnection_notfound(const track_target_ptr &targ1, const std::string &targ2) {
	msg << "Track Connection Error: Could Not Connect: " << targ1 << " to Unfound Target:" << targ2;
}

bool track_reservation_state::Reservation(EDGETYPE in_dir, unsigned int in_index, unsigned int in_rr_flags, route *resroute) {
	if(in_rr_flags & (RRF_RESERVE | RRF_TRYRESERVE)) {
		if(rr_flags & RRF_RESERVE) {	//track already reserved
			if(direction != in_dir || index != in_index) return false;	//reserved piece doesn't match
		}
		if(in_rr_flags & RRF_RESERVE) {
			rr_flags = in_rr_flags & RRF_SAVEMASK;
			direction = in_dir;
			index = in_index;
			reserved_route = resroute;
		}
		return true;
	}
	else if(in_rr_flags & RRF_UNRESERVE) {
		if(rr_flags & RRF_RESERVE) {
			if(direction != in_dir || index != in_index) return false;	//reserved piece doesn't match
			rr_flags = 0;
			direction = EDGE_NULL;
			index = 0;
			reserved_route = 0;
		}
		return true;
	}
	else return false;
}

unsigned int track_reservation_state::GetGTReservationFlags(EDGETYPE checkdirection) const {
	unsigned int outputflags = 0;
	if(rr_flags & RRF_RESERVE) {
		outputflags |= generictrack::GTF_ROUTESET;
		if(direction == checkdirection) outputflags |= generictrack::GTF_ROUTETHISDIR;
	}
	return outputflags;
}

std::ostream& operator<<(std::ostream& os, const generictrack& obj) {
	os << obj.GetFriendlyName();
	return os;
}

std::ostream& operator<<(std::ostream& os, const track_target_ptr& obj) {
	if(obj.IsValid()) os << "Track Target: " << obj.track->GetFriendlyName() << ", " << obj.direction;
	else os << "Invalid Track Target";
	return os;
}

std::ostream& operator<<(std::ostream& os, const track_location& obj) {
	if(obj.IsValid()) os << "Track Location: " << obj.GetTrack()->GetFriendlyName() << ", " << obj.GetDirection() << ", " << obj.GetOffset() << "mm";
	else os << "Invalid Track Location";
	return os;
}

struct direction_name {
	EDGETYPE dir;
	const std::string serialname;
	const std::string friendlyname;
};

const direction_name dirnames[] = {
	{ EDGE_NULL, "nulledge", "Null direction/edge" },
	{ EDGE_FRONT, "front", "Front edge/Forward direction" },
	{ EDGE_BACK, "back", "Back edge/Reverse direction" },
	{ EDGE_PTS_FACE, "facing", "Points: facing edge/input direction" },
	{ EDGE_PTS_NORMAL, "normal", "Points: normal edge/input direction" },
	{ EDGE_PTS_REVERSE, "reverse", "Points: reverse edge/input direction" },
	{ EDGE_X_N, "north", "Cross-over: North edge/input direction" },
	{ EDGE_X_S, "south", "Cross-over: South edge/input direction" },
	{ EDGE_X_W, "west", "Cross-over: West edge/input direction" },
	{ EDGE_X_E, "east", "Cross-over: East edge/input direction" },
	{ EDGE_DS_FL, "leftfront", "Double-slip: Forward direction: Front edge: Left track" },
	{ EDGE_DS_FR, "rightfront", "Double-slip: Forward direction: Front edge: Right track" },
	{ EDGE_DS_BL, "leftback", "Double-slip: Reverse direction: Back edge: Right track (seen from front)" },
	{ EDGE_DS_BR, "rightback", "Double-slip: Reverse direction: Back edge: Left track (seen from front)" },
};

std::ostream& operator<<(std::ostream& os, const EDGETYPE& obj) {
	const direction_name &dirname = dirnames[obj];
	if(dirname.dir != obj) assert(false);
	os << dirname.friendlyname;
	return os;
}
const char * SerialiseDirectionName(const EDGETYPE& obj) {
	const direction_name &dirname = dirnames[obj];
	if(dirname.dir != obj) assert(false);
	return dirname.serialname.c_str();
}

bool DeserialiseDirectionName(EDGETYPE& obj, const char *dirname) {
	for(unsigned int i = 0; i < sizeof(dirnames)/sizeof(direction_name); i++) {
		if(strcmp(dirname, dirnames[i].serialname.c_str()) == 0) {
			obj = dirnames[i].dir;
			return true;
		}
	}
	return false;
}
