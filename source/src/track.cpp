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
#include "trackcircuit.h"

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
		ec.RegisterNewError<error_trackconnection>(track_target_ptr(this, this_entrance_direction), target_entrance);
		return false;
	}
}

bool generictrack::HalfConnect(EDGETYPE this_entrance_direction, const track_target_ptr &target_entrance) {
	if(!IsEdgeValid(this_entrance_direction)) return false;
	return TryConnectPiece(GetEdgeConnectingPiece(this_entrance_direction), target_entrance);
}

class error_track_unconnected : public layout_initialisation_error_obj {
	public:
	error_track_unconnected(const track_target_ptr &edge) {
		msg << "Track edge unconnected: " << edge;
	}
};

bool generictrack::AutoConnections(error_collection &ec) {
	if(prevtrack) {
		EDGETYPE prevedge = prevtrack->GetAvailableAutoConnectionDirection(!(prevtrack->gt_privflags & GTPRIVF_REVERSEAUTOCONN));
		EDGETYPE thisedge = GetAvailableAutoConnectionDirection(gt_privflags & GTPRIVF_REVERSEAUTOCONN);
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
			ec.RegisterNewError<error_track_unconnected>(track_target_ptr(this, it->edge));
			result = false;
		}
	}

	return result;
}

bool generictrack::PostLayoutInit(error_collection &ec) {
	if(have_inited) return false;
	have_inited = true;
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

const track_target_ptr& trackseg::GetEdgeConnectingPiece(EDGETYPE edgeid) const {
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

bool trackseg::IsEdgeValid(EDGETYPE edge) const {
	switch(edge) {
		case EDGE_FRONT:
		case EDGE_BACK:
			return true;
		default:
			return false;
	}
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

bool trackseg::Reservation(EDGETYPE direction, unsigned int index, unsigned int rr_flags, const route *resroute) {
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

const track_target_ptr& crossover::GetEdgeConnectingPiece(EDGETYPE edgeid) const {
	switch(edgeid) {
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

bool crossover::IsEdgeValid(EDGETYPE edge) const {
	switch(edge) {
		case EDGE_X_N:
		case EDGE_X_S:
		case EDGE_X_W:
		case EDGE_X_E:
			return true;
		default:
			return false;
	}
}

unsigned int crossover::GetFlags(EDGETYPE direction) const {
	return GTF_ROUTEFORK | trs.GetGTReservationFlags(direction);
}

bool crossover::Reservation(EDGETYPE direction, unsigned int index, unsigned int rr_flags, const route *resroute) {
	return trs.Reservation(direction, index, rr_flags, resroute);
}

void crossover::GetListOfEdges(std::vector<edgelistitem> &outputlist) const {
	outputlist.insert(outputlist.end(), { edgelistitem(EDGE_X_N, north), edgelistitem(EDGE_X_S, south), edgelistitem(EDGE_X_W, west), edgelistitem(EDGE_X_E, east) });
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

bool track_reservation_state::Reservation(EDGETYPE in_dir, unsigned int in_index, unsigned int in_rr_flags, const route *resroute) {
	if(in_rr_flags & (RRF_RESERVE | RRF_TRYRESERVE)) {
		for(auto it = itrss.begin(); it != itrss.end(); ++it) {
			if(it->rr_flags & RRF_RESERVE) {	//track already reserved
				if(it->direction != in_dir || it->index != in_index) return false;	//reserved piece doesn't match
			}
		}
		if(in_rr_flags & RRF_RESERVE) {
			itrss.emplace_back();
			inner_track_reservation_state &itrs = itrss.back();

			itrs.rr_flags = in_rr_flags & RRF_SAVEMASK;
			itrs.direction = in_dir;
			itrs.index = in_index;
			itrs.reserved_route = resroute;
		}
		return true;
	}
	else if(in_rr_flags & RRF_UNRESERVE) {
		for(auto it = itrss.begin(); it != itrss.end(); ++it) {
			if(it->rr_flags & RRF_RESERVE && it->direction == in_dir && it->index == in_index && it->reserved_route == resroute) {
				itrss.erase(it);
				return true;
			}
		}
	}
	return false;
}

bool track_reservation_state::IsReserved() const {
	for(auto it = itrss.begin(); it != itrss.end(); ++it) {
		if(it->rr_flags & RRF_RESERVE) return true;
	}
	return false;
}

unsigned int track_reservation_state::GetReservationCount() const {
	unsigned int count = 0;
	for(auto it = itrss.begin(); it != itrss.end(); ++it) {
		if(it->rr_flags & RRF_RESERVE) count++;
	}
	return count;
}

bool track_reservation_state::IsReservedInDirection(EDGETYPE direction) const {
	for(auto it = itrss.begin(); it != itrss.end(); ++it) {
		if(it->rr_flags & RRF_RESERVE && it->direction == direction) return true;
	}
	return false;
}

unsigned int track_reservation_state::GetGTReservationFlags(EDGETYPE checkdirection) const {
	unsigned int outputflags = 0;
	if(IsReserved()) {
		outputflags |= generictrack::GTF_ROUTESET;
		if(IsReservedInDirection(checkdirection)) outputflags |= generictrack::GTF_ROUTETHISDIR;
	}
	return outputflags;
}

unsigned int track_reservation_state::ReservationEnumeration(std::function<void(const route *reserved_route, EDGETYPE direction, unsigned int index, unsigned int rr_flags)> func) const {
	unsigned int counter = 0;
	for(auto it = itrss.begin(); it != itrss.end(); ++it) {
		if(it->rr_flags & RRF_RESERVE) {
			func(it->reserved_route, it->direction, it->index, it->rr_flags);
			counter++;
		}
	}
	return counter;
}

unsigned int track_reservation_state::ReservationEnumerationInDirection(EDGETYPE direction, std::function<void(const route *reserved_route, EDGETYPE direction, unsigned int index, unsigned int rr_flags)> func) const {
	unsigned int counter = 0;
	for(auto it = itrss.begin(); it != itrss.end(); ++it) {
		if(it->rr_flags & RRF_RESERVE && it->direction == direction) {
			func(it->reserved_route, it->direction, it->index, it->rr_flags);
			counter++;
		}
	}
	return counter;
}

std::ostream& operator<<(std::ostream& os, const generictrack* obj) {
	if(obj) os << obj->GetFriendlyName();
	else os << "Null generictrack*";
	return os;
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
