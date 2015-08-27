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
#include "core/track.h"
#include "core/trackpiece.h"
#include "core/train.h"
#include "core/trackcircuit.h"

#include <cassert>
#include <cstring>
#include <initializer_list>

track_location empty_track_location;
track_target_ptr empty_track_target;

unsigned int speedrestrictionset::GetTrackSpeedLimitByClass(const std::string &speedclass, unsigned int default_max) const {
	for (auto it = speeds.begin(); it != speeds.end(); ++it) {
		if (it->speedclass.empty() || it->speedclass == speedclass) {
			if (it->speed < default_max) {
				default_max = it->speed;
			}
		}
	}
	return default_max;
}

unsigned int speedrestrictionset::GetTrainTrackSpeedLimit(const train *t /* optional */) const {
	if (t) {
		return GetTrackSpeedLimitByClass(t->GetVehSpeedClass(), t->GetMaxVehSpeed());
	} else {
		return GetTrackSpeedLimitByClass("", UINT_MAX);
	}
}

const speedrestrictionset *generictrack::GetSpeedRestrictions() const {
	return nullptr;
}

const tractionset *generictrack::GetTractionTypes() const {
	return nullptr;
}

generictrack & generictrack::SetLength(unsigned int length) {
	return *this;
}
generictrack & generictrack::SetElevationDelta(unsigned int elevationdelta) {
	return *this;
}

int generictrack::GetPartialElevationDelta(EDGETYPE direction, unsigned int displacement) const {
	int elevationdelta = GetElevationDelta(direction);
	if (elevationdelta) {
		return (((int64_t) elevationdelta) * ((int64_t) displacement)) / ((int64_t) GetLength(direction));
	}
	return 0;
}

bool generictrack::TryConnectPiece(track_target_ptr &piece_var, const track_target_ptr &new_target) {
	if (piece_var.IsValid() && piece_var != new_target) {
		return false;
	} else {
		piece_var = new_target;
		return true;
	}
}

bool generictrack::FullConnect(EDGETYPE this_entrance_direction, const track_target_ptr &target_entrance, error_collection &ec) {
	if (HalfConnect(this_entrance_direction, target_entrance) && target_entrance.track->HalfConnect(target_entrance.direction,
			track_target_ptr(this, this_entrance_direction))) {
		return true;
	} else {
		ec.RegisterNewError<error_trackconnection>(track_target_ptr(this, this_entrance_direction), target_entrance);
		return false;
	}
}

bool generictrack::HalfConnect(EDGETYPE this_entrance_direction, const track_target_ptr &target_entrance) {
	if (!IsEdgeValid(this_entrance_direction)) {
		return false;
	}
	return TryConnectPiece(GetEdgeConnectingPiece(this_entrance_direction), target_entrance);
}

class error_track_unconnected : public layout_initialisation_error_obj {
	public:
	error_track_unconnected(const track_target_ptr &edge) {
		msg << "Track edge unconnected: " << edge;
	}
};

bool generictrack::AutoConnections(error_collection &ec) {
	if (prevtrack) {
		EDGETYPE prevedge = prevtrack->GetAvailableAutoConnectionDirection(!(prevtrack->gt_privflags & GTPRIVF::REVERSEAUTOCONN));
		EDGETYPE thisedge = GetAvailableAutoConnectionDirection(gt_privflags & GTPRIVF::REVERSEAUTOCONN);
		if (prevedge != EDGE_NULL && thisedge != EDGE_NULL) {
			if (!FullConnect(thisedge, track_target_ptr(prevtrack, prevedge), ec)) {
				return false;
			}
		}
	}
	return true;
}

bool generictrack::CheckUnconnectedEdges(error_collection &ec) {
	bool result = true;

	std::vector<edgelistitem> edgelist;
	GetListOfEdges(edgelist);
	for (auto &it : edgelist) {
		if (!it.target->IsValid()) {
			ec.RegisterNewError<error_track_unconnected>(track_target_ptr(this, it.edge));
			result = false;
		}
	}

	return result;
}

bool generictrack::PostLayoutInit(error_collection &ec) {
	if (have_inited) {
		return false;
	}
	have_inited = true;
	return true;
}

bool generictrack::Reservation(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute, std::string* failreasonkey) {
	bool result = ReservationV(direction, index, rr_flags, resroute, failreasonkey);
	if (result) {
		MarkUpdated();
		UpdateTrackCircuitReservationState();
	}
	return result;
}

void generictrack::UpdateTrackCircuitReservationState() {
	if (tc) {
		reservationcountset rcs;
		ReservationTypeCount(rcs);
		if (rcs.routeset > 0 && !(gt_privflags & GTPRIVF::RESERVATION_IN_TC)) {
			tc->TrackReserved(this);
			gt_privflags |= GTPRIVF::RESERVATION_IN_TC;
		} else if (rcs.routeset == 0 && (gt_privflags & GTPRIVF::RESERVATION_IN_TC)) {
			tc->TrackUnreserved(this);
			gt_privflags &= ~GTPRIVF::RESERVATION_IN_TC;
		}
	}
}

unsigned int generictrack::ReservationEnumeration(std::function<void(const route *reserved_route, EDGETYPE direction, unsigned int index, RRF rr_flags)> func,
		RRF checkmask) {
	unsigned int counter = 0;
	std::vector<track_reservation_state *> trslist;
	GetTRSList(trslist);
	for (auto &it : trslist) {
		counter += it->ReservationEnumeration(func, checkmask);
	}
	return counter;
}

unsigned int generictrack::ReservationEnumerationInDirection(EDGETYPE direction, std::function<void(const route *reserved_route, EDGETYPE direction,
		unsigned int index, RRF rr_flags)> func, RRF checkmask) {
	unsigned int counter = 0;
	std::vector<track_reservation_state *> trslist;
	GetTRSList(trslist);
	for (auto &it : trslist) {
		counter += it->ReservationEnumerationInDirection(direction, func, checkmask);
	}
	return counter;
}

void generictrack::ReservationTypeCount(reservationcountset &rcs) const {
	std::vector<track_reservation_state *> trslist;
	const_cast<generictrack *>(this)->GetTRSList(trslist);
	for (auto &it : trslist) {
		it->ReservationTypeCount(rcs);
	}
}

void generictrack::ReservationTypeCountInDirection(reservationcountset &rcs, EDGETYPE direction) const {
	std::vector<track_reservation_state *> trslist;
	const_cast<generictrack *>(this)->GetTRSList(trslist);
	for (auto &it : trslist) {
		it->ReservationTypeCountInDirection(rcs, direction);
	}
}

unsigned int generictrack::GetSightingDistance(EDGETYPE direction) const {
	for (auto &it : sighting_distances) {
		if (it.first == direction) {
			return it.second;
		}
	}
	return 0;
}

void generictrack::TrainEnter(EDGETYPE direction, train *t) {
	track_circuit *tc = GetTrackCircuit();
	if (tc)
		tc->TrainEnter(t);
}

void generictrack::TrainLeave(EDGETYPE direction, train *t) {
	track_circuit *tc = GetTrackCircuit();
	if (tc) {
		tc->TrainLeave(t);
	}
}

void trackseg::TrainEnter(EDGETYPE direction, train *t) {
	generictrack::TrainEnter(direction, t);
	traincount++;
	occupying_trains.push_back(t);
	for (auto &it : ttcbs) {
		it->TrainEnter(t);
	}
	const speedrestrictionset *speeds = GetSpeedRestrictions();
	if (speeds) {
		t->AddCoveredTrackSpeedLimit(speeds->GetTrainTrackSpeedLimit(t));
	}
}

void trackseg::TrainLeave(EDGETYPE direction, train *t) {
	generictrack::TrainLeave(direction, t);
	traincount--;
	container_unordered_remove_if(occupying_trains, [&](const train * const it) { return it == t; });
	for (auto &it : ttcbs) {
		it->TrainLeave(t);
	}
	const speedrestrictionset *speeds = GetSpeedRestrictions();
	if (speeds) {
		t->RemoveCoveredTrackSpeedLimit(speeds->GetTrainTrackSpeedLimit(t));
	}
}

generictrack::edge_track_target trackseg::GetEdgeConnectingPiece(EDGETYPE edgeid) {
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

generictrack::edge_track_target trackseg::GetConnectingPiece(EDGETYPE direction) {
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

unsigned int trackseg::GetMaxConnectingPieces(EDGETYPE direction) const {
	return 1;
}

generictrack::edge_track_target trackseg::GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) {
	return GetConnectingPiece(direction);
}

bool trackseg::IsEdgeValid(EDGETYPE edge) const {
	switch (edge) {
		case EDGE_FRONT:
		case EDGE_BACK:
			return true;

		default:
			return false;
	}
}

int trackseg::GetElevationDelta(EDGETYPE direction) const {
	return (direction == EDGE_FRONT) ? elevationdelta : -elevationdelta;
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
	switch (direction) {
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
	switch (direction) {
		case EDGE_FRONT:
			return length - currentoffset;

		case EDGE_BACK:
			return currentoffset;

		default:
			assert(false);
			return 0;
	}
}

const std::vector<track_train_counter_block *> *trackseg::GetOtherTrackTriggers() const {
	return &ttcbs;
}

GTF trackseg::GetFlags(EDGETYPE direction) const {
	return trs.GetGTReservationFlags(direction);
}

EDGETYPE trackseg::GetReverseDirection(EDGETYPE direction) const {
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

trackseg & trackseg::SetLength(unsigned int length) {
	this->length = length;
	return *this;
}

trackseg & trackseg::SetElevationDelta(unsigned int elevationdelta) {
	this->elevationdelta = elevationdelta;
	return *this;
}

bool trackseg::ReservationV(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute, std::string* failreasonkey) {
	if (rr_flags & RRF::STOP_ON_OCCUPIED_TC && GetTrackCircuit() && GetTrackCircuit()->Occupied()) {
		return false;
	}
	return trs.Reservation(direction, index, rr_flags, resroute);
}

EDGETYPE trackseg::GetAvailableAutoConnectionDirection(bool forwardconnection) const {
	if (forwardconnection && !next.IsValid()) {
		return EDGE_BACK;
	}
	if (!forwardconnection && !prev.IsValid()) {
		return EDGE_FRONT;
	}
	return EDGE_NULL;
}

void trackseg::GetListOfEdges(std::vector<edgelistitem> &outputlist) const {
	outputlist.insert(outputlist.end(), { edgelistitem(EDGE_BACK, next), edgelistitem(EDGE_FRONT, prev) });
}

void trackseg::GetTrainOccupationState(std::vector<generictrack::train_occupation> &train_os) {
	train_os.clear();
	if (traincount == 0) {
		return;
	}

	for (train *t : occupying_trains) {
		train_os.emplace_back();
		train_occupation &to = train_os.back();

		to.t = t;
		to.start_offset = 0;
		to.end_offset = length;

		auto checkpos = [&](track_location l, bool reverse) {
			if (l.GetTrack() != this) {
				return;
			}

			if (reverse) {
				l.ReverseDirection();
			}

			//l now points *outward* from the train

			if (l.GetDirection() == EDGE_FRONT) {
				//this is the inclusive upper bound
				to.end_offset = l.GetOffset();
			}
			if (l.GetDirection() == EDGE_BACK) {
				//this is the inclusive lower bound
				to.start_offset = l.GetOffset();
			}
		};
		checkpos(t->GetTrainMotionState().head_pos, false);
		checkpos(t->GetTrainMotionState().tail_pos, true);
	}
}

unsigned int genericzlentrack::GetStartOffset(EDGETYPE direction) const {
	return 0;
}

int genericzlentrack::GetElevationDelta(EDGETYPE direction) const {
	return 0;
}

unsigned int genericzlentrack::GetNewOffset(EDGETYPE direction, unsigned int currentoffset, unsigned int step) const {
	assert(currentoffset == 0);
	return 0;
}

unsigned int genericzlentrack::GetRemainingLength(EDGETYPE direction, unsigned int currentoffset) const {
	assert(currentoffset == 0);
	return 0;
}

unsigned int genericzlentrack::GetLength(EDGETYPE direction) const {
	return 0;
}

generictrack::edge_track_target crossover::GetConnectingPiece(EDGETYPE direction) {
	switch (direction) {
		case EDGE_X_LEFT:
			return left;

		case EDGE_X_RIGHT:
			return right;

		case EDGE_FRONT:
			return front;

		case EDGE_BACK:
			return back;

		default:
			assert(false);
			return empty_track_target;
	}
}

unsigned int crossover::GetMaxConnectingPieces(EDGETYPE direction) const {
	return 1;
}

EDGETYPE crossover::GetReverseDirection(EDGETYPE direction) const {
	switch (direction) {
		case EDGE_X_LEFT:
			return EDGE_X_RIGHT;

		case EDGE_X_RIGHT:
			return EDGE_X_LEFT;

		case EDGE_FRONT:
			return EDGE_BACK;

		case EDGE_BACK:
			return EDGE_FRONT;

		default:
			assert(false);
			return EDGE_NULL;
	}
}

generictrack::edge_track_target crossover::GetEdgeConnectingPiece(EDGETYPE edgeid) {
	switch (edgeid) {
		case EDGE_X_LEFT:
			return left;

		case EDGE_X_RIGHT:
			return right;

		case EDGE_FRONT:
			return front;

		case EDGE_BACK:
			return back;

		default:
			assert(false);
			return empty_track_target;
	}
}

bool crossover::IsEdgeValid(EDGETYPE edge) const {
	switch (edge) {
		case EDGE_X_LEFT:
		case EDGE_X_RIGHT:
		case EDGE_FRONT:
		case EDGE_BACK:
			return true;

		default:
			return false;
	}
}

GTF crossover::GetFlags(EDGETYPE direction) const {
	return GTF::ROUTEFORK | trs.GetGTReservationFlags(direction);
}

bool crossover::ReservationV(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute, std::string* failreasonkey) {
	return trs.Reservation(direction, index, rr_flags, resroute);
}

void crossover::GetListOfEdges(std::vector<edgelistitem> &outputlist) const {
	outputlist.insert(outputlist.end(), { edgelistitem(EDGE_X_LEFT, left), edgelistitem(EDGE_X_RIGHT, right),
			edgelistitem(EDGE_FRONT, front), edgelistitem(EDGE_BACK, back) });
}

EDGETYPE crossover::GetAvailableAutoConnectionDirection(bool forwardconnection) const {
	if (forwardconnection && !back.IsValid()) {
		return EDGE_BACK;
	}
	if (!forwardconnection && !front.IsValid()) {
		return EDGE_FRONT;
	}
	return EDGE_NULL;
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

std::ostream& operator<<(std::ostream& os, const generictrack* obj) {
	if (obj) {
		os << obj->GetFriendlyName();
	} else {
		os << "Null generictrack*";
	}
	return os;
}

std::ostream& operator<<(std::ostream& os, const generictrack& obj) {
	os << obj.GetFriendlyName();
	return os;
}

std::ostream& StreamOutTrackPtr(std::ostream& os, const track_target_ptr& obj) {
	if (obj.IsValid()) {
		os << "Track Target: " << obj.track->GetFriendlyName() << ", " << obj.direction;
	} else {
		os << "Invalid Track Target";
	}
	return os;
}

std::ostream& operator<<(std::ostream& os, const track_location& obj) {
	if (obj.IsValid()) {
		os << "Track Location: " << obj.GetTrack()->GetFriendlyName() << ", " << obj.GetDirection() << ", " << obj.GetOffset() << "mm";
	} else {
		os << "Invalid Track Location";
	}
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
	{ EDGE_X_LEFT, "left", "Cross-over: Left edge/input direction (seen from front)" },
	{ EDGE_X_RIGHT, "right", "Cross-over: Right edge/input direction (seen from front)" },
	{ EDGE_DS_FL, "leftfront", "Double-slip: Forward direction: Front edge: Left track" },
	{ EDGE_DS_FR, "rightfront", "Double-slip: Forward direction: Front edge: Right track" },
	{ EDGE_DS_BL, "leftback", "Double-slip: Reverse direction: Back edge: Right track (seen from front)" },
	{ EDGE_DS_BR, "rightback", "Double-slip: Reverse direction: Back edge: Left track (seen from front)" },
};

std::ostream& operator<<(std::ostream& os, const EDGETYPE& obj) {
	const direction_name &dirname = dirnames[obj];
	assert(dirname.dir == obj);
	os << dirname.friendlyname;
	return os;
}
const char * SerialiseDirectionName(const EDGETYPE& obj) {
	const direction_name &dirname = dirnames[obj];
	assert(dirname.dir == obj);
	return dirname.serialname.c_str();
}

bool DeserialiseDirectionName(EDGETYPE& obj, const char *dirname) {
	for (unsigned int i = 0; i < sizeof(dirnames) / sizeof(direction_name); i++) {
		if (strcmp(dirname, dirnames[i].serialname.c_str()) == 0) {
			obj = dirnames[i].dir;
			return true;
		}
	}
	return false;
}
