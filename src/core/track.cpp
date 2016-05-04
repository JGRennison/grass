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
#include "core/track_piece.h"
#include "core/train.h"
#include "core/track_circuit.h"

#include <cassert>
#include <cstring>
#include <initializer_list>

track_location empty_track_location;
track_target_ptr empty_track_target;

unsigned int speed_restriction_set::GetTrackSpeedLimitByClass(const std::string &speed_class, unsigned int default_max) const {
	for (auto it = speeds.begin(); it != speeds.end(); ++it) {
		if (it->speed_class.empty() || it->speed_class == speed_class) {
			if (it->speed < default_max) {
				default_max = it->speed;
			}
		}
	}
	return default_max;
}

unsigned int speed_restriction_set::GetTrainTrackSpeedLimit(const train *t /* optional */) const {
	if (t) {
		return GetTrackSpeedLimitByClass(t->GetVehSpeedClass(), t->GetMaxVehSpeed());
	} else {
		return GetTrackSpeedLimitByClass("", UINT_MAX);
	}
}

const speed_restriction_set *generic_track::GetSpeedRestrictions() const {
	return nullptr;
}

const traction_set *generic_track::GetTractionTypes() const {
	return nullptr;
}

generic_track & generic_track::SetLength(unsigned int length) {
	return *this;
}
generic_track & generic_track::SetElevationDelta(unsigned int elevation_delta) {
	return *this;
}

int generic_track::GetPartialElevationDelta(EDGE direction, unsigned int displacement) const {
	int elevation_delta = GetElevationDelta(direction);
	if (elevation_delta) {
		return (((int64_t) elevation_delta) * ((int64_t) displacement)) / ((int64_t) GetLength(direction));
	}
	return 0;
}

bool generic_track::TryConnectPiece(track_target_ptr &piece_var, const track_target_ptr &new_target) {
	if (piece_var.IsValid() && piece_var != new_target) {
		return false;
	} else {
		piece_var = new_target;
		return true;
	}
}

bool generic_track::FullConnect(EDGE this_entrance_direction, const track_target_ptr &target_entrance, error_collection &ec) {
	if (HalfConnect(this_entrance_direction, target_entrance) && target_entrance.track->HalfConnect(target_entrance.direction,
			track_target_ptr(this, this_entrance_direction))) {
		return true;
	} else {
		ec.RegisterNewError<error_track_connection>(track_target_ptr(this, this_entrance_direction), target_entrance);
		return false;
	}
}

bool generic_track::HalfConnect(EDGE this_entrance_direction, const track_target_ptr &target_entrance) {
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

bool generic_track::AutoConnections(error_collection &ec) {
	if (prev_track) {
		EDGE prevedge = prev_track->GetAvailableAutoConnectionDirection(!(prev_track->gt_privflags & GTPRIVF::REVERSE_AUTO_CONN));
		EDGE thisedge = GetAvailableAutoConnectionDirection(gt_privflags & GTPRIVF::REVERSE_AUTO_CONN);
		if (prevedge != EDGE::INVALID && thisedge != EDGE::INVALID) {
			if (!FullConnect(thisedge, track_target_ptr(prev_track, prevedge), ec)) {
				return false;
			}
		}
	}
	return true;
}

bool generic_track::CheckUnconnectedEdges(error_collection &ec) {
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

bool generic_track::PostLayoutInit(error_collection &ec) {
	if (have_inited) {
		return false;
	}
	have_inited = true;
	return true;
}

reservation_result generic_track::Reservation(const reservation_request_res &req) {
	reservation_result result = ReservationV(req);
	if (result.IsSuccess()) {
		MarkUpdated();
		UpdateTrackCircuitReservationState();
	}
	return result;
}

void generic_track::UpdateTrackCircuitReservationState() {
	if (tc) {
		reservation_count_set rcs;
		ReservationTypeCount(rcs);
		if (rcs.route_set > 0 && !(gt_privflags & GTPRIVF::RESERVATION_IN_TC)) {
			tc->TrackReserved(this);
			gt_privflags |= GTPRIVF::RESERVATION_IN_TC;
		} else if (rcs.route_set == 0 && (gt_privflags & GTPRIVF::RESERVATION_IN_TC)) {
			tc->TrackUnreserved(this);
			gt_privflags &= ~GTPRIVF::RESERVATION_IN_TC;
		}
	}
}

unsigned int generic_track::ReservationEnumeration(std::function<void(const route *reserved_route, EDGE direction, unsigned int index, RRF rr_flags)> func,
		RRF check_mask) {
	unsigned int counter = 0;
	std::vector<track_reservation_state *> trslist;
	GetTRSList(trslist);
	for (auto &it : trslist) {
		counter += it->ReservationEnumeration(func, check_mask);
	}
	return counter;
}

unsigned int generic_track::ReservationEnumerationInDirection(EDGE direction, std::function<void(const route *reserved_route, EDGE direction,
		unsigned int index, RRF rr_flags)> func, RRF check_mask) {
	unsigned int counter = 0;
	std::vector<track_reservation_state *> trslist;
	GetTRSList(trslist);
	for (auto &it : trslist) {
		counter += it->ReservationEnumerationInDirection(direction, func, check_mask);
	}
	return counter;
}

void generic_track::ReservationTypeCount(reservation_count_set &rcs) const {
	std::vector<track_reservation_state *> trslist;
	const_cast<generic_track *>(this)->GetTRSList(trslist);
	for (auto &it : trslist) {
		it->ReservationTypeCount(rcs);
	}
}

void generic_track::ReservationTypeCountInDirection(reservation_count_set &rcs, EDGE direction) const {
	std::vector<track_reservation_state *> trslist;
	const_cast<generic_track *>(this)->GetTRSList(trslist);
	for (auto &it : trslist) {
		it->ReservationTypeCountInDirection(rcs, direction);
	}
}

unsigned int generic_track::GetSightingDistance(EDGE direction) const {
	for (auto &it : sighting_distances) {
		if (it.first == direction) {
			return it.second;
		}
	}
	return 0;
}

void generic_track::TrainEnter(EDGE direction, train *t) {
	track_circuit *tc = GetTrackCircuit();
	if (tc)
		tc->TrainEnter(t);
}

void generic_track::TrainLeave(EDGE direction, train *t) {
	track_circuit *tc = GetTrackCircuit();
	if (tc) {
		tc->TrainLeave(t);
	}
}

void track_seg::TrainEnter(EDGE direction, train *t) {
	generic_track::TrainEnter(direction, t);
	train_count++;
	occupying_trains.push_back(t);
	for (auto &it : ttcbs) {
		it->TrainEnter(t);
	}
	const speed_restriction_set *speeds = GetSpeedRestrictions();
	if (speeds) {
		t->AddCoveredTrackSpeedLimit(speeds->GetTrainTrackSpeedLimit(t));
	}
}

void track_seg::TrainLeave(EDGE direction, train *t) {
	generic_track::TrainLeave(direction, t);
	train_count--;
	container_unordered_remove_if(occupying_trains, [&](const train * const it) { return it == t; });
	for (auto &it : ttcbs) {
		it->TrainLeave(t);
	}
	const speed_restriction_set *speeds = GetSpeedRestrictions();
	if (speeds) {
		t->RemoveCoveredTrackSpeedLimit(speeds->GetTrainTrackSpeedLimit(t));
	}
}

generic_track::edge_track_target track_seg::GetEdgeConnectingPiece(EDGE edgeid) {
	switch (edgeid) {
		case EDGE::FRONT:
			return prev;

		case EDGE::BACK:
			return next;

		default:
			assert(false);
			return empty_track_target;
	}
}

generic_track::edge_track_target track_seg::GetConnectingPiece(EDGE direction) {
	switch (direction) {
		case EDGE::FRONT:
			return next;

		case EDGE::BACK:
			return prev;

		default:
			assert(false);
			return empty_track_target;
	}
}

unsigned int track_seg::GetMaxConnectingPieces(EDGE direction) const {
	return 1;
}

generic_track::edge_track_target track_seg::GetConnectingPieceByIndex(EDGE direction, unsigned int index) {
	return GetConnectingPiece(direction);
}

bool track_seg::IsEdgeValid(EDGE edge) const {
	switch (edge) {
		case EDGE::FRONT:
		case EDGE::BACK:
			return true;

		default:
			return false;
	}
}

int track_seg::GetElevationDelta(EDGE direction) const {
	return (direction == EDGE::FRONT) ? elevation_delta : -elevation_delta;
}

unsigned int track_seg::GetStartOffset(EDGE direction) const {
	return (direction == EDGE::BACK) ? GetLength(direction) : 0;
}

const speed_restriction_set *track_seg::GetSpeedRestrictions() const {
	return &speed_limits;
}

const traction_set *track_seg::GetTractionTypes() const {
	return &traction_types;
}

unsigned int track_seg::GetLength(EDGE direction) const {
	return length;
}

unsigned int track_seg::GetNewOffset(EDGE direction, unsigned int current_offset, unsigned int step) const {
	switch (direction) {
		case EDGE::FRONT:
			return current_offset + step;

		case EDGE::BACK:
			return current_offset - step;

		default:
			assert(false);
			return current_offset;
	}
}

unsigned int track_seg::GetRemainingLength(EDGE direction, unsigned int current_offset) const {
	switch (direction) {
		case EDGE::FRONT:
			return length - current_offset;

		case EDGE::BACK:
			return current_offset;

		default:
			assert(false);
			return 0;
	}
}

const std::vector<track_train_counter_block *> *track_seg::GetOtherTrackTriggers() const {
	return &ttcbs;
}

GTF track_seg::GetFlags(EDGE direction) const {
	return trs.GetGTReservationFlags(direction);
}

EDGE track_seg::GetReverseDirection(EDGE direction) const {
	switch (direction) {
		case EDGE::FRONT:
			return EDGE::BACK;

		case EDGE::BACK:
			return EDGE::FRONT;

		default:
			assert(false);
			return EDGE::INVALID;
	}
}

track_seg & track_seg::SetLength(unsigned int length) {
	this->length = length;
	return *this;
}

track_seg & track_seg::SetElevationDelta(unsigned int elevation_delta) {
	this->elevation_delta = elevation_delta;
	return *this;
}

reservation_result track_seg::ReservationV(const reservation_request_res &req) {
	if (req.rr_flags & RRF::STOP_ON_OCCUPIED_TC && GetTrackCircuit() && GetTrackCircuit()->Occupied()) {
		return reservation_result(RSRVRF::FAILED | RSRVRF::STOP_ON_OCCUPIED_TC);
	}
	return trs.Reservation(req);
}

EDGE track_seg::GetAvailableAutoConnectionDirection(bool forward_connection) const {
	if (forward_connection && !next.IsValid()) {
		return EDGE::BACK;
	}
	if (!forward_connection && !prev.IsValid()) {
		return EDGE::FRONT;
	}
	return EDGE::INVALID;
}

void track_seg::GetListOfEdges(std::vector<edgelistitem> &output_list) const {
	output_list.insert(output_list.end(), { edgelistitem(EDGE::BACK, next), edgelistitem(EDGE::FRONT, prev) });
}

void track_seg::GetTrainOccupationState(std::vector<generic_track::train_occupation> &train_os) {
	train_os.clear();
	if (train_count == 0) {
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

			if (l.GetDirection() == EDGE::FRONT) {
				//this is the inclusive upper bound
				to.end_offset = l.GetOffset();
			}
			if (l.GetDirection() == EDGE::BACK) {
				//this is the inclusive lower bound
				to.start_offset = l.GetOffset();
			}
		};
		checkpos(t->GetTrainMotionState().head_pos, false);
		checkpos(t->GetTrainMotionState().tail_pos, true);
	}
}

unsigned int generic_zlen_track::GetStartOffset(EDGE direction) const {
	return 0;
}

int generic_zlen_track::GetElevationDelta(EDGE direction) const {
	return 0;
}

unsigned int generic_zlen_track::GetNewOffset(EDGE direction, unsigned int current_offset, unsigned int step) const {
	assert(current_offset == 0);
	return 0;
}

unsigned int generic_zlen_track::GetRemainingLength(EDGE direction, unsigned int current_offset) const {
	assert(current_offset == 0);
	return 0;
}

unsigned int generic_zlen_track::GetLength(EDGE direction) const {
	return 0;
}

generic_track::edge_track_target crossover::GetConnectingPiece(EDGE direction) {
	switch (direction) {
		case EDGE::X_LEFT:
			return left;

		case EDGE::X_RIGHT:
			return right;

		case EDGE::FRONT:
			return front;

		case EDGE::BACK:
			return back;

		default:
			assert(false);
			return empty_track_target;
	}
}

unsigned int crossover::GetMaxConnectingPieces(EDGE direction) const {
	return 1;
}

EDGE crossover::GetReverseDirection(EDGE direction) const {
	switch (direction) {
		case EDGE::X_LEFT:
			return EDGE::X_RIGHT;

		case EDGE::X_RIGHT:
			return EDGE::X_LEFT;

		case EDGE::FRONT:
			return EDGE::BACK;

		case EDGE::BACK:
			return EDGE::FRONT;

		default:
			assert(false);
			return EDGE::INVALID;
	}
}

generic_track::edge_track_target crossover::GetEdgeConnectingPiece(EDGE edgeid) {
	switch (edgeid) {
		case EDGE::X_LEFT:
			return left;

		case EDGE::X_RIGHT:
			return right;

		case EDGE::FRONT:
			return front;

		case EDGE::BACK:
			return back;

		default:
			assert(false);
			return empty_track_target;
	}
}

bool crossover::IsEdgeValid(EDGE edge) const {
	switch (edge) {
		case EDGE::X_LEFT:
		case EDGE::X_RIGHT:
		case EDGE::FRONT:
		case EDGE::BACK:
			return true;

		default:
			return false;
	}
}

GTF crossover::GetFlags(EDGE direction) const {
	return GTF::ROUTE_FORK | trs.GetGTReservationFlags(direction);
}

reservation_result crossover::ReservationV(const reservation_request_res &req) {
	return trs.Reservation(req);
}

void crossover::GetListOfEdges(std::vector<edgelistitem> &output_list) const {
	output_list.insert(output_list.end(), { edgelistitem(EDGE::X_LEFT, left), edgelistitem(EDGE::X_RIGHT, right),
			edgelistitem(EDGE::FRONT, front), edgelistitem(EDGE::BACK, back) });
}

EDGE crossover::GetAvailableAutoConnectionDirection(bool forward_connection) const {
	if (forward_connection && !back.IsValid()) {
		return EDGE::BACK;
	}
	if (!forward_connection && !front.IsValid()) {
		return EDGE::FRONT;
	}
	return EDGE::INVALID;
}

layout_initialisation_error_obj::layout_initialisation_error_obj() {
	msg << "Track layout initialisation failure: ";
}

error_track_connection::error_track_connection(const track_target_ptr &targ1, const track_target_ptr &targ2) {
	msg << "Track Connection Error: Could Not Connect: " << targ1 << " to " << targ2;
}

error_track_connection_notfound::error_track_connection_notfound(const track_target_ptr &targ1, const std::string &targ2) {
	msg << "Track Connection Error: Could Not Connect: " << targ1 << " to Unfound Target:" << targ2;
}

std::ostream& operator<<(std::ostream& os, const generic_track* obj) {
	if (obj) {
		os << obj->GetFriendlyName();
	} else {
		os << "Null generic_track*";
	}
	return os;
}

std::ostream& operator<<(std::ostream& os, const generic_track& obj) {
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
	EDGE dir;
	const std::string serialname;
	const std::string friendlyname;
};

const direction_name dirnames[] = {
	{ EDGE::INVALID, "invalid", "Null direction/edge" },
	{ EDGE::FRONT, "front", "Front edge/Forward direction" },
	{ EDGE::BACK, "back", "Back edge/Reverse direction" },
	{ EDGE::PTS_FACE, "facing", "Points: facing edge/input direction" },
	{ EDGE::PTS_NORMAL, "normal", "Points: normal edge/input direction" },
	{ EDGE::PTS_REVERSE, "reverse", "Points: reverse edge/input direction" },
	{ EDGE::X_LEFT, "left", "Cross-over: Left edge/input direction (seen from front)" },
	{ EDGE::X_RIGHT, "right", "Cross-over: Right edge/input direction (seen from front)" },
	{ EDGE::DS_FL, "left_front", "Double-slip: Forward direction: Front edge: Left track" },
	{ EDGE::DS_FR, "right_front", "Double-slip: Forward direction: Front edge: Right track" },
	{ EDGE::DS_BL, "left_back", "Double-slip: Reverse direction: Back edge: Right track (seen from front)" },
	{ EDGE::DS_BR, "right_back", "Double-slip: Reverse direction: Back edge: Left track (seen from front)" },
};

std::ostream& operator<<(std::ostream& os, EDGE obj) {
	const direction_name &dirname = dirnames[static_cast<unsigned int>(obj)];
	assert(dirname.dir == obj);
	os << dirname.friendlyname;
	return os;
}
const char * SerialiseDirectionName(EDGE obj) {
	const direction_name &dirname = dirnames[static_cast<unsigned int>(obj)];
	assert(dirname.dir == obj);
	return dirname.serialname.c_str();
}

bool DeserialiseDirectionName(EDGE& obj, const char *dirname) {
	for (unsigned int i = 0; i < sizeof(dirnames) / sizeof(direction_name); i++) {
		if (strcmp(dirname, dirnames[i].serialname.c_str()) == 0) {
			obj = dirnames[i].dir;
			return true;
		}
	}
	return false;
}
