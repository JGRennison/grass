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

int generictrack::GetPartialElevationDelta(DIRTYPE direction, unsigned int displacement) const {
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

bool generictrack::FullConnect(DIRTYPE this_entrance_direction, const track_target_ptr &target_entrance, error_collection &ec) {
	if(HalfConnect(this_entrance_direction, target_entrance) && target_entrance.track->HalfConnect(target_entrance.direction, track_target_ptr(this, this_entrance_direction))) {
		return true;
	}
	else {
		ec.RegisterError(std::unique_ptr<error_obj>(new error_trackconnection(track_target_ptr(this, this_entrance_direction), target_entrance)));
		return false;
	}
}

void trackseg::TrainEnter(DIRTYPE direction, train *t) {
	traincount++;
	track_circuit *tc = GetTrackCircuit();
	if(tc) tc->TrainEnter(t);
	const speedrestrictionset *speeds = GetSpeedRestrictions();
	if(speeds) t->AddCoveredTrackSpeedLimit(speeds->GetTrainTrackSpeedLimit(t));
}

void trackseg::TrainLeave(DIRTYPE direction, train *t) {
	traincount++;
	track_circuit *tc = GetTrackCircuit();
	if(tc) tc->TrainEnter(t);
	const speedrestrictionset *speeds = GetSpeedRestrictions();
	t->RemoveCoveredTrackSpeedLimit(speeds->GetTrainTrackSpeedLimit(t));
}

bool trackseg::HalfConnect(DIRTYPE this_entrance_direction, const track_target_ptr &target_entrance) {
	switch(this_entrance_direction) {
		case TDIR_FORWARD:
			return TryConnectPiece(prev, target_entrance);
		case TDIR_REVERSE:
			return TryConnectPiece(next, target_entrance);
		default:
			return false;
	}
}

const track_target_ptr & trackseg::GetConnectingPiece(DIRTYPE direction) const {
	switch(direction) {
		case TDIR_FORWARD:
			return next;
		case TDIR_REVERSE:
			return prev;
		default:
			assert(false);
			return empty_track_target;
	}
}

unsigned int trackseg::GetMaxConnectingPieces(DIRTYPE direction) const {
	return 1;
}
const track_target_ptr & trackseg::GetConnectingPieceByIndex(DIRTYPE direction, unsigned int index) const {
	return GetConnectingPiece(direction);
}

int trackseg::GetElevationDelta(DIRTYPE direction) const {
	return (direction==TDIR_FORWARD) ? elevationdelta : -elevationdelta;
}

unsigned int trackseg::GetStartOffset(DIRTYPE direction) const {
	return (direction == TDIR_REVERSE) ? GetLength(direction) : 0;
}

const speedrestrictionset *trackseg::GetSpeedRestrictions() const {
	return &speed_limits;
}

const tractionset *trackseg::GetTractionTypes() const {
	return &tractiontypes;
}

unsigned int trackseg::GetLength(DIRTYPE direction) const {
	return length;
}

unsigned int trackseg::GetNewOffset(DIRTYPE direction, unsigned int currentoffset, unsigned int step) const {
	switch(direction) {
		case TDIR_FORWARD:
			return currentoffset + step;
		case TDIR_REVERSE:
			return currentoffset - step;
		default:
			assert(false);
			return currentoffset;
	}
}

unsigned int trackseg::GetRemainingLength(DIRTYPE direction, unsigned int currentoffset) const {
	switch(direction) {
		case TDIR_FORWARD:
			return length - currentoffset;
		case TDIR_REVERSE:
			return currentoffset;
		default:
			assert(false);
			return 0;
	}
}

track_circuit *trackseg::GetTrackCircuit() const {
	return tc;
}

unsigned int trackseg::GetFlags(DIRTYPE direction) const {
	return trs.GetGTReservationFlags(direction);
}

DIRTYPE trackseg::GetReverseDirection(DIRTYPE direction) const {
	switch(direction) {
		case TDIR_FORWARD:
			return TDIR_REVERSE;
		case TDIR_REVERSE:
			return TDIR_FORWARD;
		default:
			assert(false);
			return TDIR_NULL;
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

bool trackseg::Reservation(DIRTYPE direction, unsigned int index, unsigned int rr_flags, route *resroute) {
	return trs.Reservation(direction, index, rr_flags, resroute);
}

unsigned int genericzlentrack::GetStartOffset(DIRTYPE direction) const {
	return 0;
}

int genericzlentrack::GetElevationDelta(DIRTYPE direction) const {
	return 0;
}

unsigned int genericzlentrack::GetNewOffset(DIRTYPE direction, unsigned int currentoffset, unsigned int step) const {
	if(currentoffset > 0) assert(false);
	return 0;
}

unsigned int genericzlentrack::GetRemainingLength(DIRTYPE direction, unsigned int currentoffset) const {
	if(currentoffset > 0) assert(false);
	return 0;
}

unsigned int genericzlentrack::GetLength(DIRTYPE direction) const {
	return 0;
}

void genericpoints::TrainEnter(DIRTYPE direction, train *t) { }
void genericpoints::TrainLeave(DIRTYPE direction, train *t) { }


const track_target_ptr & points::GetConnectingPiece(DIRTYPE direction) const {
	if(IsOOC(0)) return empty_track_target;

	bool pointsrev = pflags & PTF_REV;

	switch(direction) {
		case TDIR_PTS_FACE:
			return pointsrev ? reverse : normal;
		case TDIR_PTS_NORMAL:
			return pointsrev ? empty_track_target : prev;
		case TDIR_PTS_REVERSE:
			return pointsrev ? prev : empty_track_target;
		default:
			assert(false);
			return empty_track_target;
	}
}

unsigned int points::GetMaxConnectingPieces(DIRTYPE direction) const {
	switch(direction) {
		case TDIR_PTS_FACE:
			return 2;
		case TDIR_PTS_NORMAL:
			return 1;
		case TDIR_PTS_REVERSE:
			return 1;
		default:
			assert(false);
			return 0;
	}
}

const track_target_ptr & points::GetConnectingPieceByIndex(DIRTYPE direction, unsigned int index) const {
	switch(direction) {
		case TDIR_PTS_FACE:
			return index == 1 ? reverse : normal;
		case TDIR_PTS_NORMAL:
			return prev;
		case TDIR_PTS_REVERSE:
			return prev;
		default:
			assert(false);
			return empty_track_target;
	}
}

DIRTYPE points::GetReverseDirection(DIRTYPE direction) const {
	if(IsOOC(0)) return TDIR_NULL;

	bool pointsrev = pflags & PTF_REV;

	switch(direction) {
		case TDIR_PTS_FACE:
			return pointsrev ? TDIR_PTS_REVERSE : TDIR_PTS_NORMAL;
		case TDIR_PTS_NORMAL:
			return pointsrev ? TDIR_NULL : TDIR_PTS_FACE;
		case TDIR_PTS_REVERSE:
			return pointsrev ? TDIR_PTS_FACE : TDIR_NULL;
		default:
			assert(false);
			return TDIR_NULL;
	}
}

bool points::HalfConnect(DIRTYPE this_entrance_direction, const track_target_ptr &target_entrance) {
	switch(this_entrance_direction) {
		case TDIR_PTS_FACE:
			return TryConnectPiece(prev, target_entrance);
		case TDIR_PTS_NORMAL:
			return TryConnectPiece(normal, target_entrance);
		case TDIR_PTS_REVERSE:
			return TryConnectPiece(reverse, target_entrance);
		default:
			return false;
	}
}

unsigned int points::GetFlags(DIRTYPE direction) const {
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

bool IsPointsRoutingDirAndIndexRev(DIRTYPE direction, unsigned int index) {
	switch(direction) {
		case TDIR_PTS_FACE:
			return (index == 1);
		case TDIR_PTS_NORMAL:
			return false;
		case TDIR_PTS_REVERSE:
			return true;
		default:
			return false;
	}
}

bool points::Reservation(DIRTYPE direction, unsigned int index, unsigned int rr_flags, route *resroute) {
	unsigned int pflags = GetPointFlags(0);
	bool rev = pflags & PTF_REV;
	if(pflags & (PTF_LOCKED | PTF_REMINDER)) {
		if(IsPointsRoutingDirAndIndexRev(direction, index) != rev) return false;
	}
	return trs.Reservation(direction, index, rr_flags, resroute);
}

void points::ReservationActions(DIRTYPE direction, unsigned int index, unsigned int rr_flags, route *resroute, world &w, std::function<void(action &&reservation_act)> submitaction) {
	if(rr_flags & RRF_RESERVE) {
		bool rev = pflags & PTF_REV;
		bool newrev = IsPointsRoutingDirAndIndexRev(direction, index);
		if(newrev != rev) {
			submitaction(action_pointsaction(w, *this, 0, newrev ? PTF_REV : 0, PTF_REV));
		}
	}
}

const track_target_ptr & catchpoints::GetConnectingPiece(DIRTYPE direction) const {
	if(IsOOC(0)) return empty_track_target;
	if(pflags & PTF_REV) return empty_track_target;

	switch(direction) {
		case TDIR_FORWARD:
			return next;
		case TDIR_REVERSE:
			return prev;
		default:
			assert(false);
			return empty_track_target;
	}
}

unsigned int catchpoints::GetMaxConnectingPieces(DIRTYPE direction) const {
	switch(direction) {
		case TDIR_FORWARD:
			return 1;
		case TDIR_REVERSE:
			return 1;
		default:
			assert(false);
			return 0;
	}
}

const track_target_ptr & catchpoints::GetConnectingPieceByIndex(DIRTYPE direction, unsigned int index) const {
	switch(direction) {
		case TDIR_FORWARD:
			return next;
		case TDIR_REVERSE:
			return prev;
		default:
			assert(false);
			return empty_track_target;
	}
}

DIRTYPE catchpoints::GetReverseDirection(DIRTYPE direction) const {
	if(IsOOC(0)) return TDIR_NULL;
	if(pflags & PTF_REV) return TDIR_NULL;

	switch(direction) {
		case TDIR_FORWARD:
			return TDIR_REVERSE;
		case TDIR_REVERSE:
			return TDIR_FORWARD;
		default:
			assert(false);
			return TDIR_NULL;
	}
}

bool catchpoints::HalfConnect(DIRTYPE this_entrance_direction, const track_target_ptr &target_entrance) {
	switch(this_entrance_direction) {
		case TDIR_FORWARD:
			return TryConnectPiece(prev, target_entrance);
		case TDIR_REVERSE:
			return TryConnectPiece(next, target_entrance);
		default:
			return false;
	}
}

unsigned int catchpoints::GetFlags(DIRTYPE direction) const {
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

bool catchpoints::Reservation(DIRTYPE direction, unsigned int index, unsigned int rr_flags, route *resroute) {
	unsigned int pflags = GetPointFlags(0);
	if(pflags & PTF_LOCKED && pflags & PTF_REV)  return false;
	return trs.Reservation(direction, index, rr_flags, resroute);
}

void catchpoints::ReservationActions(DIRTYPE direction, unsigned int index, unsigned int rr_flags, route *resroute, world &w, std::function<void(action &&reservation_act)> submitaction) {
	if(rr_flags & RRF_RESERVE) {
		submitaction(action_pointsaction(w, *this, 0, 0, PTF_REV));
	}
	else if(rr_flags & RRF_UNRESERVE) {
		submitaction(action_pointsaction(w, *this, 0, PTF_REV, PTF_REV));
	}
}

const track_target_ptr & springpoints::GetConnectingPiece(DIRTYPE direction) const {
	switch(direction) {
		case TDIR_PTS_FACE:
			return sendreverse ? reverse : normal;
		case TDIR_PTS_NORMAL:
			return prev;
		case TDIR_PTS_REVERSE:
			return prev;
		default:
			assert(false);
			return empty_track_target;
	}
}

unsigned int springpoints::GetMaxConnectingPieces(DIRTYPE direction) const {
	return 1;
}

DIRTYPE springpoints::GetReverseDirection(DIRTYPE direction) const {
	switch(direction) {
		case TDIR_PTS_FACE:
			return sendreverse ? TDIR_PTS_REVERSE : TDIR_PTS_NORMAL;
		case TDIR_PTS_NORMAL:
			return TDIR_PTS_FACE;
		case TDIR_PTS_REVERSE:
			return TDIR_PTS_FACE;
		default:
			assert(false);
			return TDIR_NULL;
	}
}

bool springpoints::HalfConnect(DIRTYPE this_entrance_direction, const track_target_ptr &target_entrance) {
	switch(this_entrance_direction) {
		case TDIR_PTS_FACE:
			return TryConnectPiece(prev, target_entrance);
		case TDIR_PTS_NORMAL:
			return TryConnectPiece(normal, target_entrance);
		case TDIR_PTS_REVERSE:
			return TryConnectPiece(reverse, target_entrance);
		default:
			return false;
	}
}

unsigned int springpoints::GetFlags(DIRTYPE direction) const {
	return GTF_ROUTEFORK | trs.GetGTReservationFlags(direction);
}

bool springpoints::Reservation(DIRTYPE direction, unsigned int index, unsigned int rr_flags, route *resroute) {
	return trs.Reservation(direction, index, rr_flags, resroute);
}

const track_target_ptr & crossover::GetConnectingPiece(DIRTYPE direction) const {
	switch(direction) {
		case TDIR_X_N:
			return north;
		case TDIR_X_S:
			return south;
		case TDIR_X_W:
			return west;
		case TDIR_X_E:
			return east;
		default:
			assert(false);
			return empty_track_target;
	}
}

unsigned int crossover::GetMaxConnectingPieces(DIRTYPE direction) const {
	return 1;
}

DIRTYPE crossover::GetReverseDirection(DIRTYPE direction) const {
	switch(direction) {
		case TDIR_X_N:
			return TDIR_X_S;
		case TDIR_X_S:
			return TDIR_X_N;
		case TDIR_X_W:
			return TDIR_X_E;
		case TDIR_X_E:
			return TDIR_X_W;
		default:
			assert(false);
			return TDIR_NULL;
	}
}

bool crossover::HalfConnect(DIRTYPE this_entrance_direction, const track_target_ptr &target_entrance) {
	switch(this_entrance_direction) {
		case TDIR_X_N:
			return TryConnectPiece(north, target_entrance);
		case TDIR_X_S:
			return TryConnectPiece(south, target_entrance);
		case TDIR_X_W:
			return TryConnectPiece(west, target_entrance);
		case TDIR_X_E:
			return TryConnectPiece(east, target_entrance);
		default:
			return false;
	}
}

unsigned int crossover::GetFlags(DIRTYPE direction) const {
	return GTF_ROUTEFORK | trs.GetGTReservationFlags(direction);
}

bool crossover::Reservation(DIRTYPE direction, unsigned int index, unsigned int rr_flags, route *resroute) {
	return trs.Reservation(direction, index, rr_flags, resroute);
}

const track_target_ptr & doubleslip::GetConnectingPiece(DIRTYPE direction) const {
	DIRTYPE exitdirection = GetReverseDirection(direction);

	if(exitdirection != TDIR_NULL) return GetInputPiece(exitdirection);
	else return empty_track_target;
}

unsigned int doubleslip::GetMaxConnectingPieces(DIRTYPE direction) const {
	switch(direction) {
		case TDIR_DS_FL:
			return 2 - __builtin_popcount(dsflags&(DSF_NO_FL_RR | DSF_NO_FL_RL));
		case TDIR_DS_FR:
			return 2 - __builtin_popcount(dsflags&(DSF_NO_FR_RR | DSF_NO_FR_RL));
		case TDIR_DS_RL:
			return 2 - __builtin_popcount(dsflags&(DSF_NO_FL_RL | DSF_NO_FR_RL));
		case TDIR_DS_RR:
			return 2 - __builtin_popcount(dsflags&(DSF_NO_FL_RR | DSF_NO_FR_RR));
		default:
			assert(false);
			return 0;
	}
}

const track_target_ptr & doubleslip::GetConnectingPieceByIndex(DIRTYPE direction, unsigned int index) const {
	unsigned int pf = GetCurrentPointFlags(direction);
	bool isrev = (pf & PTF_FIXED) ? (pf & PTF_REV) : index;

	DIRTYPE exitdirection = GetConnectingPointDirection(direction, isrev);
	return GetInputPiece(exitdirection);
}

DIRTYPE doubleslip::GetReverseDirection(DIRTYPE direction) const {
	unsigned int pf = GetCurrentPointFlags(direction);
	if(IsFlagsOOC(pf)) return TDIR_NULL;

	DIRTYPE exitdirection = GetConnectingPointDirection(direction, pf & PTF_REV);
	unsigned int exitpf = GetCurrentPointFlags(exitdirection);
	if(IsFlagsOOC(exitpf)) return TDIR_NULL;

	if(GetConnectingPointDirection(exitdirection, exitpf & PTF_REV) == direction) {
		return exitdirection;
	}
	else return TDIR_NULL;
}

bool doubleslip::HalfConnect(DIRTYPE this_entrance_direction, const track_target_ptr &target_entrance) {
	track_target_ptr &piece = GetInputPiece(this_entrance_direction);
	if(piece.IsValid()) return TryConnectPiece(piece, target_entrance);
	else return false;
}

unsigned int doubleslip::GetFlags(DIRTYPE direction) const {
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

bool doubleslip::Reservation(DIRTYPE direction, unsigned int index, unsigned int rr_flags, route *resroute) {
	unsigned int pf = GetCurrentPointFlags(direction);
	if(pf & PTF_LOCKED) {
		if(!(pf & PTF_REV) != !index) return false;	//points locked in wrong direction
	}

	bool isrev = (pf & PTF_FIXED) ? (pf & PTF_REV) : index;
	DIRTYPE exitdirection = GetConnectingPointDirection(direction, isrev);
	unsigned int exitpf = GetCurrentPointFlags(exitdirection);
	bool exitpointsrev = (GetConnectingPointDirection(exitdirection, true) == direction);

	if(exitpf & PTF_LOCKED) {
		if(!(exitpf & PTF_REV) != !exitpointsrev) return false;	//points locked in wrong direction
	}

	return trs.Reservation(direction, index, rr_flags, resroute);
}

void doubleslip::ReservationActions(DIRTYPE direction, unsigned int index, unsigned int rr_flags, route *resroute, world &w, std::function<void(action &&reservation_act)> submitaction) {
	if(rr_flags & RRF_RESERVE) {
		unsigned int entranceindex = GetCurrentPointIndex(direction);
		unsigned int entrancepf = GetCurrentPointFlags(direction);
		bool isentrancerev = (entrancepf & PTF_FIXED) ? (entrancepf & PTF_REV) : index;

		DIRTYPE exitdirection = GetConnectingPointDirection(direction, isentrancerev);

		unsigned int exitindex = GetCurrentPointIndex(exitdirection);
		unsigned int exitpf = GetCurrentPointFlags(exitdirection);
		
		bool isexitrev = (GetConnectingPointDirection(exitdirection, true) == direction);

		if(!(entrancepf & PTF_REV) != !(isentrancerev)) submitaction(action_pointsaction(w, *this, entranceindex, isentrancerev ? PTF_REV : 0, PTF_REV));
		if(!(exitpf & PTF_REV) != !(isexitrev)) submitaction(action_pointsaction(w, *this, exitindex, isexitrev ? PTF_REV : 0, PTF_REV));
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

bool track_reservation_state::Reservation(DIRTYPE in_dir, unsigned int in_index, unsigned int in_rr_flags, route *resroute) {
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
			direction = TDIR_NULL;
			index = 0;
			reserved_route = 0;
		}
		return true;
	}
	else return false;
}

unsigned int track_reservation_state::GetGTReservationFlags(DIRTYPE checkdirection) const {
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
	DIRTYPE dir;
	const std::string serialname;
	const std::string friendlyname;
};

const direction_name dirnames[] = {
	{ TDIR_NULL, "nulldirection", "Null direction" },
	{ TDIR_FORWARD, "forward", "Forward direction" },
	{ TDIR_REVERSE, "reverse", "Reverse direction" },
	{ TDIR_PTS_FACE, "facing", "Points: facing input direction" },
	{ TDIR_PTS_NORMAL, "normal", "Points: facing input direction" },
	{ TDIR_PTS_REVERSE, "reverse", "Points: reverse input direction" },
	{ TDIR_X_N, "north", "Cross-over: North face input direction" },
	{ TDIR_X_S, "south", "Cross-over: South face input direction" },
	{ TDIR_X_W, "west", "Cross-over: West face input direction" },
	{ TDIR_X_E, "east", "Cross-over: East face input direction" },
	{ TDIR_DS_FL, "leftforward", "Double-slip: Forward direction: Left track" },
	{ TDIR_DS_FR, "rightforward", "Double-slip: Forward direction: Right track" },
	{ TDIR_DS_RL, "leftreverse", "Double-slip: Reverse direction: Left track" },
	{ TDIR_DS_RR, "rightreverse", "Double-slip: Reverse direction: Right track" },
};

std::ostream& operator<<(std::ostream& os, const DIRTYPE& obj) {
	const direction_name &dirname = dirnames[obj];
	if(dirname.dir != obj) assert(false);
	os << dirname.friendlyname;
	return os;
}
const char * SerialiseDirectionName(const DIRTYPE& obj) {
	const direction_name &dirname = dirnames[obj];
	if(dirname.dir != obj) assert(false);
	return dirname.serialname.c_str();
}

bool DeserialiseDirectionName(DIRTYPE& obj, const char *dirname) {
	for(unsigned int i = 0; i < sizeof(dirnames)/sizeof(direction_name); i++) {
		if(strcmp(dirname, dirnames[i].serialname.c_str()) == 0) {
			obj = dirnames[i].dir;
			return true;
		}
	}
	return false;
}
