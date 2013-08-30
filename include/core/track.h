//  grass - Generic Rail And Signalling Simulator
//
//  WEBSITE: http://bitbucket.org/JGRennison/grass
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

#ifndef INC_TRACK_ALREADY
#define INC_TRACK_ALREADY

#include <forward_list>
#include <string>
#include <iostream>
#include <array>
#include <vector>
#include <limits.h>

#include "flags.h"
#include "edgetype.h"
#include "world_obj.h"
#include "tractiontype.h"
#include "error.h"

class train;
class generictrack;
template <typename T> struct vartrack_target_ptr;
typedef vartrack_target_ptr<generictrack> track_target_ptr;
class track_location;
class route;
class action;
class world_serialisation;
class track_circuit;
class track_train_counter_block;
class track_reservation_state;
enum class RRF : unsigned int;
enum class GTF : unsigned int;

struct speed_restriction {
	unsigned int speed;
	std::string speedclass;
};

class speedrestrictionset : public serialisable_obj {
	std::forward_list<speed_restriction> speeds;

	public:
	unsigned int GetTrackSpeedLimitByClass(const std::string &speedclass, unsigned int default_max) const;
	unsigned int GetTrainTrackSpeedLimit(const train *t /* optional */) const;
	inline void AddSpeedRestriction(const speed_restriction &sr) {
		speeds.push_front(sr);
	}
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

void DeserialiseRouteTargetByParentAndIndex(const route *& output, const deserialiser_input &di, error_collection &ec, bool after_layout_init_resolve=false);

struct trackberth {
	std::string contents;
	EDGETYPE direction = EDGE_NULL;
};

class generictrack : public world_obj {
	generictrack *prevtrack;
	bool have_inited = false;

	enum class GTPRIVF {
		ZERO		= 0,
		REVERSEAUTOCONN	= 1<<0,
	};
	GTPRIVF gt_privflags = GTPRIVF::ZERO;

	std::unique_ptr<trackberth> berth;

	friend world_serialisation;
	void SetPreviousTrackPiece(generictrack *prev) { prevtrack = prev; }

	protected:
	std::vector<std::pair<EDGETYPE, unsigned int> > sighting_distances;

	public:
	generictrack(world &w_) : world_obj(w_), prevtrack(0) { }
	virtual const speedrestrictionset *GetSpeedRestrictions() const;
	virtual const tractionset *GetTractionTypes() const;
	virtual unsigned int GetNewOffset(EDGETYPE direction, unsigned int currentoffset, unsigned int step) const = 0;
	virtual unsigned int GetStartOffset(EDGETYPE direction) const = 0;
	virtual unsigned int GetRemainingLength(EDGETYPE direction, unsigned int currentoffset) const = 0;
	virtual unsigned int GetLength(EDGETYPE direction) const = 0;
	virtual int GetElevationDelta(EDGETYPE direction) const = 0;
	virtual int GetPartialElevationDelta(EDGETYPE direction, unsigned int displacement) const;
	virtual track_circuit *GetTrackCircuit() const;
	virtual const std::vector<track_train_counter_block *> *GetOtherTrackTriggers() const { return 0; }
	virtual void TrainEnter(EDGETYPE direction, train *t) = 0;
	virtual void TrainLeave(EDGETYPE direction, train *t) = 0;
	virtual EDGETYPE GetReverseDirection(EDGETYPE direction) const = 0;

	virtual const track_target_ptr & GetEdgeConnectingPiece(EDGETYPE edgeid) const = 0;
	virtual const track_target_ptr & GetConnectingPiece(EDGETYPE direction) const = 0;
	virtual unsigned int GetMaxConnectingPieces(EDGETYPE direction) const = 0;
	virtual const track_target_ptr & GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) const = 0;
	virtual unsigned int GetCurrentNominalConnectionIndex(EDGETYPE direction) const { return 0; }
	unsigned int GetSightingDistance(EDGETYPE direction) const;

	inline track_target_ptr & GetEdgeConnectingPiece(EDGETYPE edgeid) {  return const_cast<track_target_ptr &>(const_cast<const generictrack*>(this)->GetEdgeConnectingPiece(edgeid)); }

	bool FullConnect(EDGETYPE this_entrance_direction, const track_target_ptr &target_entrance, error_collection &ec);

	protected:
	//return true if reservation OK
	virtual bool ReservationV(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute, std::string* failreasonkey = 0) = 0;
	virtual void ReservationActionsV(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute, std::function<void(action &&reservation_act)> submitaction) { }

	public:
	bool Reservation(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute, std::string* failreasonkey = 0) {
		bool result = ReservationV(direction, index, rr_flags, resroute, failreasonkey);
		if(result) MarkUpdated();
		return result;
	}
	void ReservationActions(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute, std::function<void(action &&reservation_act)> submitaction) {
		ReservationActionsV(direction, index, rr_flags, resroute, submitaction);
	}

	virtual std::string GetTypeName() const { return "Generic Track"; }
	static std::string GetTypeSerialisationClassNameStatic() { return "track"; }
	virtual std::string GetTypeSerialisationClassName() const override { return GetTypeSerialisationClassNameStatic(); }

	virtual generictrack & SetLength(unsigned int length);
	virtual generictrack & AddSpeedRestriction(speed_restriction sr);
	virtual generictrack & SetElevationDelta(unsigned int elevationdelta);
	virtual generictrack & SetTrackCircuit(track_circuit *tc);

	virtual bool PostLayoutInit(error_collection &ec);	//return false to discontinue initing piece
	inline bool IsPostLayoutInitDone() const { return have_inited; }
	virtual bool AutoConnections(error_collection &ec);
	virtual bool CheckUnconnectedEdges(error_collection &ec);

	virtual EDGETYPE GetDefaultValidDirecton() const = 0;

	virtual GTF GetFlags(EDGETYPE direction) const = 0;

	void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	void Serialise(serialiser_output &so, error_collection &ec) const override;
	virtual std::string GetTypeSerialisationName() const = 0;

	virtual void TrackTick() { }

	virtual bool IsTrackAlwaysPassable() const { return true; }
	inline bool IsTrackPassable(EDGETYPE direction, unsigned int connection_index) const;

	unsigned int ReservationEnumeration(std::function<void(const route *reserved_route, EDGETYPE direction, unsigned int index, RRF rr_flags)> func, RRF checkmask);
	unsigned int ReservationEnumerationInDirection(EDGETYPE direction, std::function<void(const route *reserved_route, EDGETYPE direction, unsigned int index, RRF rr_flags)> func, RRF checkmask);

	struct edgelistitem {
		EDGETYPE edge;
		const track_target_ptr *target;
		edgelistitem(EDGETYPE e, const track_target_ptr &t) : edge(e), target(&t) { }
		edgelistitem(const edgelistitem &in) : edge(in.edge), target(in.target) { }
	};
	virtual void GetListOfEdges(std::vector<edgelistitem> &outputlist) const = 0;
	virtual bool IsEdgeValid(EDGETYPE edge) const = 0;

	protected:
	virtual EDGETYPE GetAvailableAutoConnectionDirection(bool forwardconnection) const = 0;
	bool HalfConnect(EDGETYPE this_entrance_direction, const track_target_ptr &target_entrance);

	virtual unsigned int GetTRSList(std::vector<track_reservation_state *> &outputlist) { return 0; }

	virtual bool CanHaveBerth() const { return false; }

	public:
	inline bool HasBerth() { return static_cast<bool>(berth); }
	inline bool HasBerth(EDGETYPE direction) { return HasBerth() && (GetBerth()->direction == EDGE_NULL || GetBerth()->direction == direction); }
	inline trackberth *GetBerth() { return berth.get(); }

	private:
	static bool TryConnectPiece(track_target_ptr &piece_var, const track_target_ptr &new_target);
};
template<> struct enum_traits< generictrack::GTPRIVF > {	static constexpr bool flags = true; };

template <typename T> struct vartrack_target_ptr {
	T *track;
	EDGETYPE direction;
	inline bool IsValid() const { return track; }
	inline void ReverseDirection() {
		direction = track->GetReverseDirection(direction);
	}
	inline void Reset() {
		track = 0;
		direction = EDGE_NULL;
	}
	vartrack_target_ptr() { Reset(); }
	vartrack_target_ptr(T *track_, EDGETYPE direction_) : track(track_), direction(direction_) { }
	vartrack_target_ptr(const vartrack_target_ptr &in) : track(in.track), direction(in.direction) { }
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
	inline operator vartrack_target_ptr<generictrack>() const {
		return vartrack_target_ptr<generictrack>(track, direction);
	}
	const vartrack_target_ptr &GetConnectingPiece() const;

	void Deserialise(const std::string &name, const deserialiser_input &di, error_collection &ec);
	void Serialise(const std::string &name, serialiser_output &so, error_collection &ec) const;
};

extern const track_location empty_track_location;
extern const track_target_ptr empty_track_target;

template<> inline const vartrack_target_ptr<generictrack> &vartrack_target_ptr<generictrack>::GetConnectingPiece() const {
	return track ? track->GetConnectingPiece(direction) : empty_track_target;
}

inline bool generictrack::IsTrackPassable(EDGETYPE direction, unsigned int connection_index) const {
	return GetConnectingPiece(direction) == GetConnectingPieceByIndex(direction, connection_index);
}

class track_location {
	track_target_ptr trackpiece;
	unsigned int offset;

	public:
	inline bool IsValid() const { return trackpiece.IsValid(); }
	inline void ReverseDirection() {
		trackpiece.ReverseDirection();
	}
	inline void SetTargetStartLocation(const track_target_ptr &targ) {
		trackpiece = targ;
		offset = trackpiece.track ? trackpiece.track->GetStartOffset(trackpiece.direction) : 0;
	}
	inline void SetTargetStartLocation(generictrack *t, EDGETYPE dir) {
		SetTargetStartLocation(track_target_ptr(t, dir));
	}
	inline const EDGETYPE &GetDirection() const {
		return trackpiece.direction;
	}
	inline generictrack * const &GetTrack() const {
		return trackpiece.track;
	}
	inline const unsigned int &GetOffset() const {
		return offset;
	}
	inline const track_target_ptr &GetTrackTargetPtr() const {
		return trackpiece;
	}
	inline EDGETYPE &GetDirection() {
		return trackpiece.direction;
	}
	inline generictrack *&GetTrack() {
		return trackpiece.track;
	}
	inline unsigned int &GetOffset() {
		return offset;
	}
	inline track_target_ptr &GetTrackTargetPtr() {
		return trackpiece;
	}
	inline bool operator==(const track_location &other) const  {
		return trackpiece == other.trackpiece && offset == other.offset;
	}
	inline bool operator!=(const track_location &other) const  {
		return !(*this == other);
	}

	track_location() : trackpiece(), offset(0) { }
	track_location(const track_target_ptr &targ) {
		SetTargetStartLocation(targ);
	}
	track_location(generictrack *track_, EDGETYPE direction_, unsigned int offset_) : trackpiece(track_, direction_), offset(offset_) { }

	void Deserialise(const std::string &name, const deserialiser_input &di, error_collection &ec);
	void Serialise(const std::string &name, serialiser_output &so, error_collection &ec) const;
};

class genericzlentrack : public generictrack {
	public:
	genericzlentrack(world &w_) : generictrack(w_) { }
	virtual unsigned int GetNewOffset(EDGETYPE direction, unsigned int currentoffset, unsigned int step) const override;
	virtual unsigned int GetRemainingLength(EDGETYPE direction, unsigned int currentoffset) const override;
	virtual unsigned int GetLength(EDGETYPE direction) const override;
	virtual unsigned int GetStartOffset(EDGETYPE direction) const override;
	virtual int GetElevationDelta(EDGETYPE direction) const override;
};

class layout_initialisation_error_obj : public error_obj {
	public:
	layout_initialisation_error_obj();
};

class error_trackconnection : public layout_initialisation_error_obj {
	public:
	error_trackconnection(const track_target_ptr &targ1, const track_target_ptr &targ2);
};

class error_trackconnection_notfound : public layout_initialisation_error_obj {
	public:
	error_trackconnection_notfound(const track_target_ptr &targ1, const std::string &targ2);
};

std::ostream& operator<<(std::ostream& os, const generictrack& obj);
std::ostream& operator<<(std::ostream& os, const generictrack* obj);
std::ostream& StreamOutTrackPtr(std::ostream& os, const track_target_ptr& obj);
template <typename C> std::ostream& operator<<(std::ostream& os, const vartrack_target_ptr<C>& obj) { return StreamOutTrackPtr(os, obj); }
std::ostream& operator<<(std::ostream& os, const track_location& obj);

#endif
