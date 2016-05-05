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

#ifndef INC_TRACK_ALREADY
#define INC_TRACK_ALREADY

#include <forward_list>
#include <string>
#include <iostream>
#include <array>
#include <vector>
#include <limits.h>

#include "util/error.h"
#include "util/flags.h"
#include "core/edge_type.h"
#include "core/world_obj.h"
#include "core/traction_type.h"

class train;
class generic_track;
template <typename T> struct vartrack_target_ptr;
typedef vartrack_target_ptr<generic_track> track_target_ptr;
typedef vartrack_target_ptr<const generic_track> const_track_target_ptr;
template <typename T> class vartrack_location;
typedef vartrack_location<generic_track> track_location;
typedef vartrack_location<const generic_track> const_track_location;
class route;
class action;
class world_deserialisation;
class track_circuit;
class track_train_counter_block;
class track_reservation_state;
enum class RRF : unsigned int;
enum class GTF : unsigned int;
struct reservation_count_set;
struct reservation_result;
struct reservation_request_res;
struct reservation_request_action;

struct speed_restriction {
	unsigned int speed;
	std::string speed_class;
};

class speed_restriction_set : public serialisable_obj {
	std::forward_list<speed_restriction> speeds;

	public:
	unsigned int GetTrackSpeedLimitByClass(const std::string &speed_class, unsigned int default_max) const;
	unsigned int GetTrainTrackSpeedLimit(const train *t /* optional */) const;
	inline void AddSpeedRestriction(const speed_restriction &sr) {
		speeds.push_front(sr);
	}
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

void DeserialiseRouteTargetByParentAndIndex(const route *& output, const deserialiser_input &di, error_collection &ec,
		bool after_layout_init_resolve = false);
void SerialiseRouteTargetByParentAndIndex(const route *rt, serialiser_output &so, error_collection &ec);

struct track_berth {
	std::string contents;
	EDGE direction = EDGE::INVALID;
};

class generic_track : public world_obj {
	generic_track *prev_track = nullptr;
	generic_track *next_track = nullptr;
	track_circuit *tc = nullptr;
	bool have_inited = false;

	enum class GTPRIVF {
		ZERO                = 0,
		REVERSE_AUTO_CONN   = 1<<0,     ///< Reverse auto connection
		RESERVATION_IN_TC   = 1<<1,     ///< The reservation counter of the track circuit has been incremented
	};
	GTPRIVF gt_privflags = GTPRIVF::ZERO;

	std::unique_ptr<track_berth> berth;

	unsigned int connection_reserved_edge_mask = 0;

	friend world_deserialisation;
	void SetPreviousTrackPiece(generic_track *prev) { prev_track = prev; }
	void SetNextTrackPiece(generic_track *next) { next_track = next; }
	public:
	const generic_track *GetPreviousTrackPiece() const { return prev_track; }
	const generic_track *GetNextTrackPiece() const { return next_track; }

	typedef track_target_ptr & edge_track_target;
	typedef const const_track_target_ptr const_edge_track_target;

	protected:
	std::vector<std::pair<EDGE, unsigned int> > sighting_distances;

	public:
	generic_track(world &w_) : world_obj(w_) { }
	virtual const speed_restriction_set *GetSpeedRestrictions() const;
	virtual const traction_set *GetTractionTypes() const;
	virtual unsigned int GetNewOffset(EDGE direction, unsigned int current_offset, unsigned int step) const = 0;
	virtual unsigned int GetStartOffset(EDGE direction) const = 0;
	virtual unsigned int GetRemainingLength(EDGE direction, unsigned int current_offset) const = 0;
	virtual unsigned int GetLength(EDGE direction) const = 0;
	virtual int GetElevationDelta(EDGE direction) const = 0;
	virtual int GetPartialElevationDelta(EDGE direction, unsigned int displacement) const;
	virtual const std::vector<track_train_counter_block *> *GetOtherTrackTriggers() const { return nullptr; }
	virtual void TrainEnter(EDGE direction, train *t);
	virtual void TrainLeave(EDGE direction, train *t);
	virtual EDGE GetReverseDirection(EDGE direction) const = 0;

	virtual edge_track_target GetEdgeConnectingPiece(EDGE edgeid) = 0;
	const_edge_track_target GetEdgeConnectingPiece(EDGE edgeid) const;
	virtual edge_track_target GetConnectingPiece(EDGE direction) = 0;
	const_edge_track_target GetConnectingPiece(EDGE edgeid) const;
	virtual unsigned int GetMaxConnectingPieces(EDGE direction) const = 0;
	virtual edge_track_target GetConnectingPieceByIndex(EDGE direction, unsigned int index) = 0;
	const_edge_track_target GetConnectingPieceByIndex(EDGE edgeid, unsigned int index) const;
	virtual unsigned int GetCurrentNominalConnectionIndex(EDGE direction) const { return 0; }
	unsigned int GetSightingDistance(EDGE direction) const;

	inline track_circuit *GetTrackCircuit() { return tc; }
	inline const track_circuit *GetTrackCircuit() const { return tc; }

	bool FullConnect(EDGE this_entrance_direction, const track_target_ptr &target_entrance, error_collection &ec);

	protected:
	//return true if reservation OK
	virtual reservation_result ReservationV(const reservation_request_res &req) = 0;
	virtual void ReservationActionsV(const reservation_request_action &req) { }
	void DeserialiseReservationState(track_reservation_state &trs, const deserialiser_input &di, const char *name, error_collection &ec);

	inline bool ConnectionIsEdgeReserved(EDGE edge) const {
		return connection_reserved_edge_mask & (1 << static_cast<unsigned int>(edge));
	}

	private:
	inline void ConnectionReserveEdge(EDGE edge) {
		connection_reserved_edge_mask |= (1 << static_cast<unsigned int>(edge));
	}
	inline void ConnectionUnreserveEdge(EDGE edge) {
		connection_reserved_edge_mask &= ~(1 << static_cast<unsigned int>(edge));
	}

	public:
	void UpdateTrackCircuitReservationState();
	reservation_result Reservation(const reservation_request_res &req);

	inline void ReservationActions(const reservation_request_action &req) {
		ReservationActionsV(req);
	}

	virtual std::string GetTypeName() const { return "Generic Track"; }
	static std::string GetTypeSerialisationClassNameStatic() { return "track"; }
	virtual std::string GetTypeSerialisationClassName() const override { return GetTypeSerialisationClassNameStatic(); }

	virtual bool PostLayoutInit(error_collection &ec);    //return false to discontinue initing piece
	inline bool IsPostLayoutInitDone() const { return have_inited; }
	virtual bool AutoConnections(error_collection &ec);
	virtual bool CheckUnconnectedEdges(error_collection &ec);

	virtual EDGE GetDefaultValidDirecton() const = 0;

	virtual GTF GetFlags(EDGE direction) const = 0;

	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	virtual void TrackTick() { }

	virtual bool IsTrackAlwaysPassable() const { return true; }
	inline bool IsTrackPassable(EDGE direction, unsigned int connection_index) const;

	unsigned int ReservationEnumeration(std::function<void(const route *reserved_route, EDGE direction, unsigned int index, RRF rr_flags)> func,
			RRF check_mask);
	unsigned int ReservationEnumerationInDirection(EDGE direction, std::function<void(const route *reserved_route, EDGE direction,
			unsigned int index, RRF rr_flags)> func, RRF check_mask);
	void ReservationTypeCount(reservation_count_set &rcs) const;
	void ReservationTypeCountInDirection(reservation_count_set &rcs, EDGE direction) const;

	struct edgelistitem {
		EDGE edge;
		const track_target_ptr *target;
		edgelistitem(EDGE e, const track_target_ptr &t) : edge(e), target(&t) { }
		edgelistitem(const edgelistitem &in) : edge(in.edge), target(in.target) { }
	};
	virtual void GetListOfEdges(std::vector<edgelistitem> &output_list) const = 0;
	virtual bool IsEdgeValid(EDGE edge) const = 0;

	protected:
	virtual EDGE GetAvailableAutoConnectionDirection(bool forward_connection) const = 0;
	bool HalfConnect(EDGE this_entrance_direction, const track_target_ptr &target_entrance);

	virtual unsigned int GetTRSList(std::vector<track_reservation_state *> &output_list) { return 0; }

	virtual bool CanHaveBerth() const { return false; }

	public:
	inline bool HasBerth() { return static_cast<bool>(berth); }
	inline bool HasBerth(EDGE direction) { return HasBerth() && (GetBerth()->direction == EDGE::INVALID || GetBerth()->direction == direction); }
	inline track_berth *GetBerth() { return berth.get(); }

	struct train_occupation {
		train *t;
		unsigned int start_offset; //inclusive
		unsigned int end_offset;   //inclusive
	};
	virtual void GetTrainOccupationState(std::vector<train_occupation> &train_os) {
		train_os.clear();
	}

	private:
	static bool TryConnectPiece(track_target_ptr &piece_var, const track_target_ptr &new_target);


	public:
	// For use of test code **only**
	virtual generic_track & SetLength(unsigned int length);
	virtual generic_track & SetElevationDelta(unsigned int elevation_delta);
};
template<> struct enum_traits< generic_track::GTPRIVF > { static constexpr bool flags = true; };

template <typename T> struct vartrack_target_ptr {
	T *track;
	EDGE direction;

	inline bool IsValid() const { return static_cast<bool>(track); }
	inline void ReverseDirection() {
		direction = track->GetReverseDirection(direction);
	}
	inline void Reset() {
		track = nullptr;
		direction = EDGE::INVALID;
	}
	vartrack_target_ptr() { Reset(); }
	vartrack_target_ptr(T *track_, EDGE direction_) : track(track_), direction(direction_) { }
	template <typename S>
	vartrack_target_ptr(const vartrack_target_ptr<S> &in) : track(in.track), direction(in.direction) { }
	template <typename S> inline bool operator==(const vartrack_target_ptr<S> &other) const {
		return track == other.track && direction == other.direction;
	}
	template <typename S> inline bool operator!=(const vartrack_target_ptr<S> &other) const {
		return !(*this == other);
	}
	template <typename S> inline void operator=(const vartrack_target_ptr<S> &other) {
		track = other.track;
		direction = other.direction;
	}
	const vartrack_target_ptr &GetConnectingPiece() const;

	void Deserialise(const std::string &name, const deserialiser_input &di, error_collection &ec);
	void Serialise(const std::string &name, serialiser_output &so, error_collection &ec) const;
};

extern track_location empty_track_location;
extern track_target_ptr empty_track_target;

template<> inline const vartrack_target_ptr<generic_track> &vartrack_target_ptr<generic_track>::GetConnectingPiece() const {
	return track ? track->GetConnectingPiece(direction) : empty_track_target;
}

inline bool generic_track::IsTrackPassable(EDGE direction, unsigned int connection_index) const {
	return GetConnectingPiece(direction) == GetConnectingPieceByIndex(direction, connection_index);
}

template <typename T> class vartrack_location {
	vartrack_target_ptr<T> track_piece;
	unsigned int offset;

	public:
	inline bool IsValid() const { return track_piece.IsValid(); }
	inline void ReverseDirection() {
		track_piece.ReverseDirection();
	}
	inline void SetTargetStartLocation(const vartrack_target_ptr<T> &targ) {
		track_piece = targ;
		offset = track_piece.track ? track_piece.track->GetStartOffset(track_piece.direction) : 0;
	}
	inline void SetTargetStartLocation(generic_track *t, EDGE dir) {
		SetTargetStartLocation(vartrack_target_ptr<T>(t, dir));
	}
	inline const EDGE &GetDirection() const {
		return track_piece.direction;
	}
	inline T * const &GetTrack() const {
		return track_piece.track;
	}
	inline const unsigned int &GetOffset() const {
		return offset;
	}
	inline const vartrack_target_ptr<T> &GetTrackTargetPtr() const {
		return track_piece;
	}
	inline EDGE &GetDirection() {
		return track_piece.direction;
	}
	inline T *&GetTrack() {
		return track_piece.track;
	}
	inline unsigned int &GetOffset() {
		return offset;
	}
	inline vartrack_target_ptr<T> &GetTrackTargetPtr() {
		return track_piece;
	}
	inline bool operator==(const track_location &other) const {
		return track_piece == other.track_piece && offset == other.offset;
	}
	inline bool operator!=(const track_location &other) const {
		return !(*this == other);
	}

	vartrack_location() : track_piece(), offset(0) { }
	vartrack_location(const vartrack_target_ptr<T> &targ) {
		SetTargetStartLocation(targ);
	}
	vartrack_location(T *track_, EDGE direction_, unsigned int offset_) : track_piece(track_, direction_), offset(offset_) { }

	void Deserialise(const std::string &name, const deserialiser_input &di, error_collection &ec);
	void Serialise(const std::string &name, serialiser_output &so, error_collection &ec) const;
};

class generic_zlen_track : public generic_track {
	public:
	generic_zlen_track(world &w_) : generic_track(w_) { }
	virtual unsigned int GetNewOffset(EDGE direction, unsigned int current_offset, unsigned int step) const override;
	virtual unsigned int GetRemainingLength(EDGE direction, unsigned int current_offset) const override;
	virtual unsigned int GetLength(EDGE direction) const override;
	virtual unsigned int GetStartOffset(EDGE direction) const override;
	virtual int GetElevationDelta(EDGE direction) const override;
};

class layout_initialisation_error_obj : public error_obj {
	public:
	layout_initialisation_error_obj();
};

class error_track_connection : public layout_initialisation_error_obj {
	public:
	error_track_connection(const track_target_ptr &targ1, const track_target_ptr &targ2);
};

class error_track_connection_notfound : public layout_initialisation_error_obj {
	public:
	error_track_connection_notfound(const track_target_ptr &targ1, const std::string &targ2);
};

std::ostream& operator<<(std::ostream& os, const generic_track& obj);
std::ostream& operator<<(std::ostream& os, const generic_track* obj);
std::ostream& StreamOutTrackPtr(std::ostream& os, const track_target_ptr& obj);
template <typename C> std::ostream& operator<<(std::ostream& os, const vartrack_target_ptr<C>& obj) { return StreamOutTrackPtr(os, obj); }
std::ostream& operator<<(std::ostream& os, const track_location& obj);

inline generic_track::const_edge_track_target generic_track::GetEdgeConnectingPiece(EDGE edgeid) const {
	return const_cast<generic_track*>(this)->GetEdgeConnectingPiece(edgeid);
}
inline generic_track::const_edge_track_target generic_track::GetConnectingPiece(EDGE edgeid) const  {
	return const_cast<generic_track*>(this)->GetConnectingPiece(edgeid);
}
inline generic_track::const_edge_track_target generic_track::GetConnectingPieceByIndex(EDGE edgeid, unsigned int index) const  {
	return const_cast<generic_track*>(this)->GetConnectingPieceByIndex(edgeid, index);
}

#endif
