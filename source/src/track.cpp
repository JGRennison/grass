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

#include "../include/common.h"
#include "../include/track.h"
#include "../include/train.h"

#include <cassert>

const track_location empty_track_location;
const track_target_ptr empty_track_target;

unsigned int speedrestrictionset::GetTrainTrackSpeedLimit(train *t) const {
	unsigned int max_speed = t->GetMaxVehSpeed();
	for(auto it = speeds.begin(); it != speeds.end(); ++it) {
		if(it->speedclass == 0 || it->speedclass == t->GetVehSpeedClass()) {
			if(it->speed < max_speed) max_speed = it->speed;
		}
	}
	return max_speed;
}

track_circuit *generictrack::GetTrackCircuit() const {
	return 0;
}

unsigned int generictrack::GetFlags(DIRTYPE direction) const {
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
		assert(false);
		return false;
	}
	else {
		piece_var = new_target;
		return true;
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
			assert(false);
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
	return 0;
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
	if(pflags & PTF_OOC) return empty_track_target;
	
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
	if(pflags & PTF_OOC) return TDIR_NULL;
	
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
			assert(false);
			return false;
	}
}

unsigned int points::GetFlags(DIRTYPE direction) const {
	return GTF_ROUTEFORK;
}

unsigned int points::GetPointFlags() const {
	return pflags;
}

unsigned int points::SetPointFlagsMasked(unsigned int set_flags, unsigned int mask_flags) {
	pflags = (pflags & (~mask_flags)) | set_flags;
	return pflags;
}

std::string generictrack::GetFriendlyName() const {
	std::string result= std::string(GetTypeName()).append(": ").append((!name.empty()) ? name : std::string("[unnamed]"));
	return result;
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
std::ostream& operator<<(std::ostream& os, const DIRTYPE& obj) {
	switch(obj) {
		case TDIR_NULL:
			os << "Null direction";
			break;
		case TDIR_FORWARD:
			os << "Forward direction";
			break;
		case TDIR_REVERSE:
			os << "Reverse direction";
			break;
		case TDIR_PTS_FACE:
			os << "Points: facing input direction";
			break;
		case TDIR_PTS_NORMAL:
			os << "Points: normal input direction";
			break;
		case TDIR_PTS_REVERSE:
			os << "Points: reverse input direction";
			break;
		default:
			os << "Unknown direction";
			break;	
	}
	return os;
}