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

#ifndef INC_TRACK_ALREADY
#define INC_TRACK_ALREADY

#include <forward_list>
#include <string>
#include <iostream>
#include <array>
#include <vector>
#include <limits.h>

#include "world_obj.h"
#include "trackcircuit.h"
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

struct speed_restriction {
	unsigned int speed;
	std::string speedclass;
};

class speedrestrictionset : public serialisable_obj {
	std::forward_list<speed_restriction> speeds;

	public:
	unsigned int GetTrackSpeedLimitByClass(const std::string &speedclass, unsigned int default_max) const;
	unsigned int GetTrainTrackSpeedLimit(train *t) const;
	inline void AddSpeedRestriction(const speed_restriction &sr) {
		speeds.push_front(sr);
	}
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec);
	virtual void Serialise(serialiser_output &so, error_collection &ec) const;
};

typedef enum {
	EDGE_NULL = 0,
	EDGE_FRONT,		//front edge/forward direction on track
	EDGE_BACK,		//back edge/reverse direction on track

	EDGE_PTS_FACE,		//points: facing direction
	EDGE_PTS_NORMAL,	//points: normal trailing direction
	EDGE_PTS_REVERSE,	//points: reverse trailing direction

	EDGE_X_N,		//crossover: North face (connects South)
	EDGE_X_S,		//crossover: South face (connects North)
	EDGE_X_W,		//crossover: West face (connects East)
	EDGE_X_E,		//crossover: East face (connects West)

	EDGE_DS_FL,		//double-slip: front edge, left track
	EDGE_DS_FR,		//double-slip: front edge, right track
	EDGE_DS_BL,		//double-slip: back edge, left track (as seen from front)
	EDGE_DS_BR,		//double-slip: back edge, right track (as seen from front)
} EDGETYPE;

enum {
	RRF_RESERVE			= 1<<0,
	RRF_UNRESERVE			= 1<<1,
	RRF_AUTOROUTE			= 1<<2,
	RRF_TRYRESERVE			= 1<<3,
	RRF_STARTPIECE			= 1<<4,
	RRF_ENDPIECE			= 1<<5,

	RRF_SAVEMASK			= RRF_AUTOROUTE | RRF_STARTPIECE | RRF_ENDPIECE | RRF_RESERVE,
};

struct track_reservation_state : public serialisable_obj {
	route *reserved_route = 0;
	EDGETYPE direction = EDGE_NULL;
	unsigned int index = 0;
	unsigned int rr_flags = 0;

	bool Reservation(EDGETYPE direction, unsigned int index, unsigned int rr_flags, route *resroute);
	unsigned int GetGTReservationFlags(EDGETYPE direction) const;

	virtual void Deserialise(const deserialiser_input &di, error_collection &ec);
	virtual void Serialise(serialiser_output &so, error_collection &ec) const;
};

class generictrack : public world_obj {
	generictrack *prevtrack;
	unsigned int gt_flags;

	enum {
		GTF_REVERSEAUTOCONN	= 1<<0,
	};

	friend world_serialisation;
	void SetPreviousTrackPiece(generictrack *prev) { prevtrack = prev; }

	public:
	generictrack(world &w_) : world_obj(w_), prevtrack(0), gt_flags(0) { }
	virtual const speedrestrictionset *GetSpeedRestrictions() const;
	virtual const tractionset *GetTractionTypes() const;
	virtual const track_target_ptr & GetConnectingPiece(EDGETYPE direction) const = 0;
	virtual unsigned int GetNewOffset(EDGETYPE direction, unsigned int currentoffset, unsigned int step) const = 0;
	virtual unsigned int GetStartOffset(EDGETYPE direction) const = 0;
	virtual unsigned int GetRemainingLength(EDGETYPE direction, unsigned int currentoffset) const = 0;
	virtual unsigned int GetLength(EDGETYPE direction) const = 0;
	virtual int GetElevationDelta(EDGETYPE direction) const = 0;
	virtual int GetPartialElevationDelta(EDGETYPE direction, unsigned int displacement) const;
	virtual track_circuit *GetTrackCircuit() const;
	virtual unsigned int GetFlags(EDGETYPE direction) const = 0;
	virtual void TrainEnter(EDGETYPE direction, train *t) = 0;
	virtual void TrainLeave(EDGETYPE direction, train *t) = 0;
	virtual EDGETYPE GetReverseDirection(EDGETYPE direction) const = 0;

	virtual unsigned int GetMaxConnectingPieces(EDGETYPE direction) const = 0;
	virtual const track_target_ptr & GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) const = 0;

	bool FullConnect(EDGETYPE this_entrance_direction, const track_target_ptr &target_entrance, error_collection &ec);

	//return true if reservation OK
	virtual bool Reservation(EDGETYPE direction, unsigned int index, unsigned int rr_flags, route *resroute) = 0;
	virtual void ReservationActions(EDGETYPE direction, unsigned int index, unsigned int rr_flags, route *resroute, world &w, std::function<void(action &&reservation_act)> submitaction) { }

	virtual std::string GetTypeName() const { return "Generic Track"; }
	static std::string GetTypeSerialisationClassNameStatic() { return "track"; }
	virtual std::string GetTypeSerialisationClassName() const { return GetTypeSerialisationClassNameStatic(); }

	virtual generictrack & SetLength(unsigned int length);
	virtual generictrack & AddSpeedRestriction(speed_restriction sr);
	virtual generictrack & SetElevationDelta(unsigned int elevationdelta);
	virtual generictrack & SetTrackCircuit(track_circuit *tc);

	virtual bool PostLayoutInit(error_collection &ec);	//return false to discontinue initing piece
	virtual bool AutoConnections(error_collection &ec);
	virtual bool CheckUnconnectedEdges(error_collection &ec);

	virtual EDGETYPE GetDefaultValidDirecton() const = 0;

	enum {
		GTF_ROUTESET	= 1<<0,
		GTF_ROUTETHISDIR= 1<<1,
		GTF_ROUTEFORK	= 1<<2,
		GTF_ROUTINGPOINT= 1<<3,
	};

	void Deserialise(const deserialiser_input &di, error_collection &ec);
	virtual std::string GetTypeSerialisationName() const = 0;

	protected:
	virtual bool HalfConnect(EDGETYPE this_entrance_direction, const track_target_ptr &target_entrance) = 0;
	static bool TryConnectPiece(track_target_ptr &piece_var, const track_target_ptr &new_target);
	virtual EDGETYPE GetAvailableAutoConnectionDirection(bool forwardconnection) const = 0;
	
	struct edgelistitem {
		EDGETYPE edge;
		const track_target_ptr *target;
		edgelistitem(EDGETYPE e, const track_target_ptr &t) : edge(e), target(&t) { }
		edgelistitem(const edgelistitem &in) : edge(in.edge), target(in.target) { }
	};
	virtual void GetListOfEdges(std::vector<edgelistitem> &outputlist) const = 0;
};

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
	inline bool operator==(const vartrack_target_ptr &other) const {
		return track == other.track && direction == other.direction;
	}
	inline bool operator!=(const vartrack_target_ptr &other) const {
		return !(*this == other);
	}
	inline void operator=(const vartrack_target_ptr &other) {
		track = other.track;
		direction = other.direction;
	}
	template <typename S> inline operator vartrack_target_ptr<S>() {
		return vartrack_target_ptr<S>(track, direction);
	}
	const vartrack_target_ptr &GetConnectingPiece() const;
};

extern const track_location empty_track_location;
extern const track_target_ptr empty_track_target;

template<> inline const vartrack_target_ptr<generictrack> &vartrack_target_ptr<generictrack>::GetConnectingPiece() const {
	return track ? track->GetConnectingPiece(direction) : empty_track_target;
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
	inline generictrack * const &GetTrack() const  {
		return trackpiece.track;
	}
	inline const unsigned int &GetOffset() const  {
		return offset;
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
};

class trackseg : public generictrack {
	unsigned int length;
	int elevationdelta;
	speedrestrictionset speed_limits;
	tractionset tractiontypes;
	track_circuit *tc;
	unsigned int traincount;
	track_target_ptr next;
	track_target_ptr prev;
	track_reservation_state trs;

	public:
	trackseg(world &w_) : generictrack(w_), length(0), elevationdelta(0), tc(0), traincount(0) { }
	void TrainEnter(EDGETYPE direction, train *t);
	void TrainLeave(EDGETYPE direction, train *t);
	const track_target_ptr & GetConnectingPiece(EDGETYPE direction) const;
	unsigned int GetStartOffset(EDGETYPE direction) const;
	int GetElevationDelta(EDGETYPE direction) const;
	unsigned int GetLength(EDGETYPE direction) const;
	const speedrestrictionset *GetSpeedRestrictions() const;
	const tractionset *GetTractionTypes() const;
	unsigned int GetNewOffset(EDGETYPE direction, unsigned int currentoffset, unsigned int step) const;
	unsigned int GetRemainingLength(EDGETYPE direction, unsigned int currentoffset) const;
	track_circuit *GetTrackCircuit() const;
	unsigned int GetFlags(EDGETYPE direction) const;
	EDGETYPE GetReverseDirection(EDGETYPE direction) const;
	virtual EDGETYPE GetDefaultValidDirecton() const { return EDGE_FRONT; }

	unsigned int GetMaxConnectingPieces(EDGETYPE direction) const;
	const track_target_ptr & GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) const;
	virtual bool Reservation(EDGETYPE direction, unsigned int index, unsigned int rr_flags, route *resroute);

	virtual std::string GetTypeName() const { return "Track Segment"; }

	trackseg & SetLength(unsigned int length);
	trackseg & AddSpeedRestriction(speed_restriction sr);
	trackseg & SetElevationDelta(unsigned int elevationdelta);
	trackseg & SetTrackCircuit(track_circuit *tc);

	static std::string GetTypeSerialisationNameStatic() { return "trackseg"; }
	virtual std::string GetTypeSerialisationName() const { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec);
	virtual void Serialise(serialiser_output &so, error_collection &ec) const;

	protected:
	bool HalfConnect(EDGETYPE this_entrance_direction, const track_target_ptr &target_entrance);
	virtual EDGETYPE GetAvailableAutoConnectionDirection(bool forwardconnection) const;
	virtual void GetListOfEdges(std::vector<edgelistitem> &outputlist) const;
};

class genericzlentrack : public generictrack {
	public:
	genericzlentrack(world &w_) : generictrack(w_) { }
	unsigned int GetNewOffset(EDGETYPE direction, unsigned int currentoffset, unsigned int step) const;
	unsigned int GetRemainingLength(EDGETYPE direction, unsigned int currentoffset) const;
	unsigned int GetLength(EDGETYPE direction) const;
	unsigned int GetStartOffset(EDGETYPE direction) const;
	int GetElevationDelta(EDGETYPE direction) const;
};

class genericpoints : public genericzlentrack {
	public:
	enum {
		PTF_REV		= 1<<0,
		PTF_OOC		= 1<<1,
		PTF_LOCKED	= 1<<2,
		PTF_REMINDER	= 1<<3,
		PTF_FAILEDNORM	= 1<<4,
		PTF_FAILEDREV	= 1<<5,
		PTF_INVALID	= 1<<6,
		PTF_FIXED	= 1<<7,
		PTF_SERIALISABLE = PTF_REV | PTF_OOC | PTF_LOCKED | PTF_REMINDER | PTF_FAILEDNORM | PTF_FAILEDREV,
	};
	genericpoints(world &w_) : genericzlentrack(w_) { }
	void TrainEnter(EDGETYPE direction, train *t);
	void TrainLeave(EDGETYPE direction, train *t);
	virtual unsigned int GetPointCount() const = 0;
	virtual unsigned int GetPointFlags(unsigned int points_index) const = 0;
	virtual unsigned int SetPointFlagsMasked(unsigned int points_index, unsigned int set_flags, unsigned int mask_flags)  = 0;

	virtual std::string GetTypeName() const { return "Generic Points"; }

	inline bool IsFlagsOOC(unsigned int pflags) const {
		if(pflags & PTF_OOC) return true;
		if(pflags & PTF_REV && pflags & PTF_FAILEDREV) return true;
		if(!(pflags & PTF_REV) && pflags & PTF_FAILEDNORM) return true;
		return false;
	}

	inline bool IsOOC(unsigned int points_index) const {
		return IsFlagsOOC(GetPointFlags(points_index));
	}
};

class points : public genericpoints {
	track_target_ptr prev;
	track_target_ptr normal;
	track_target_ptr reverse;
	unsigned int pflags;
	track_reservation_state trs;

	public:
	points(world &w_) : genericpoints(w_), pflags(0) { }

	const track_target_ptr & GetConnectingPiece(EDGETYPE direction) const;
	EDGETYPE GetReverseDirection(EDGETYPE direction) const;
	unsigned int GetFlags(EDGETYPE direction) const;
	virtual EDGETYPE GetDefaultValidDirecton() const { return EDGE_PTS_FACE; }

	unsigned int GetMaxConnectingPieces(EDGETYPE direction) const;
	const track_target_ptr & GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) const;
	virtual bool Reservation(EDGETYPE direction, unsigned int index, unsigned int rr_flags, route *resroute);
	virtual void ReservationActions(EDGETYPE direction, unsigned int index, unsigned int rr_flags, route *resroute, world &w, std::function<void(action &&reservation_act)> submitaction);

	virtual std::string GetTypeName() const { return "Points"; }

	virtual unsigned int GetPointCount() const { return 1; }
	virtual unsigned int GetPointFlags(unsigned int points_index) const;
	virtual unsigned int SetPointFlagsMasked(unsigned int points_index, unsigned int set_flags, unsigned int mask_flags);

	static std::string GetTypeSerialisationNameStatic() { return "points"; }
	virtual std::string GetTypeSerialisationName() const { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec);
	virtual void Serialise(serialiser_output &so, error_collection &ec) const;

	protected:
	bool HalfConnect(EDGETYPE this_entrance_direction, const track_target_ptr &target_entrance);
	virtual EDGETYPE GetAvailableAutoConnectionDirection(bool forwardconnection) const;
	virtual void GetListOfEdges(std::vector<edgelistitem> &outputlist) const;
};

class catchpoints : public genericpoints {
	track_target_ptr prev;
	track_target_ptr next;
	unsigned int pflags;
	track_reservation_state trs;

	public:
	catchpoints(world &w_) : genericpoints(w_), pflags(PTF_REV) { }

	const track_target_ptr & GetConnectingPiece(EDGETYPE direction) const;
	EDGETYPE GetReverseDirection(EDGETYPE direction) const;
	unsigned int GetFlags(EDGETYPE direction) const;
	virtual EDGETYPE GetDefaultValidDirecton() const { return EDGE_PTS_FACE; }

	unsigned int GetMaxConnectingPieces(EDGETYPE direction) const;
	const track_target_ptr & GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) const;
	virtual bool Reservation(EDGETYPE direction, unsigned int index, unsigned int rr_flags, route *resroute);
	virtual void ReservationActions(EDGETYPE direction, unsigned int index, unsigned int rr_flags, route *resroute, world &w, std::function<void(action &&reservation_act)> submitaction);

	virtual std::string GetTypeName() const { return "Catch Points"; }

	virtual unsigned int GetPointCount() const { return 1; }
	virtual unsigned int GetPointFlags(unsigned int points_index) const;
	virtual unsigned int SetPointFlagsMasked(unsigned int points_index, unsigned int set_flags, unsigned int mask_flags);

	static std::string GetTypeSerialisationNameStatic() { return "catchpoints"; }
	virtual std::string GetTypeSerialisationName() const { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec);
	virtual void Serialise(serialiser_output &so, error_collection &ec) const;

	protected:
	bool HalfConnect(EDGETYPE this_entrance_direction, const track_target_ptr &target_entrance);
	virtual EDGETYPE GetAvailableAutoConnectionDirection(bool forwardconnection) const;
	virtual void GetListOfEdges(std::vector<edgelistitem> &outputlist) const;
};

class springpoints : public genericzlentrack {
	track_target_ptr prev;
	track_target_ptr normal;
	track_target_ptr reverse;
	bool sendreverse;
	track_reservation_state trs;

	public:
	springpoints(world &w_) : genericzlentrack(w_), sendreverse(false) { }
	void TrainEnter(EDGETYPE direction, train *t) { }
	void TrainLeave(EDGETYPE direction, train *t) { }

	const track_target_ptr & GetConnectingPiece(EDGETYPE direction) const;
	EDGETYPE GetReverseDirection(EDGETYPE direction) const;
	unsigned int GetFlags(EDGETYPE direction) const;
	virtual EDGETYPE GetDefaultValidDirecton() const { return EDGE_PTS_FACE; }

	unsigned int GetMaxConnectingPieces(EDGETYPE direction) const;
	const track_target_ptr & GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) const { return GetConnectingPiece(direction); }
	virtual bool Reservation(EDGETYPE direction, unsigned int index, unsigned int rr_flags, route *resroute);

	virtual std::string GetTypeName() const { return "Spring Points"; }

	static std::string GetTypeSerialisationNameStatic() { return "springpoints"; }
	virtual std::string GetTypeSerialisationName() const { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec);
	virtual void Serialise(serialiser_output &so, error_collection &ec) const;

	bool GetSendReverseFlag() const { return sendreverse; }
	void SetSendReverseFlag(bool sr) { sendreverse = sr; }

	protected:
	bool HalfConnect(EDGETYPE this_entrance_direction, const track_target_ptr &target_entrance);
	virtual EDGETYPE GetAvailableAutoConnectionDirection(bool forwardconnection) const;
	virtual void GetListOfEdges(std::vector<edgelistitem> &outputlist) const;
};

class crossover : public genericzlentrack {
	track_target_ptr north;
	track_target_ptr south;
	track_target_ptr west;
	track_target_ptr east;
	track_reservation_state trs;

	public:
	crossover(world &w_) : genericzlentrack(w_) { }
	void TrainEnter(EDGETYPE direction, train *t) { }
	void TrainLeave(EDGETYPE direction, train *t) { }

	const track_target_ptr & GetConnectingPiece(EDGETYPE direction) const;
	EDGETYPE GetReverseDirection(EDGETYPE direction) const;
	unsigned int GetFlags(EDGETYPE direction) const;
	virtual EDGETYPE GetDefaultValidDirecton() const { return EDGE_X_N; }

	unsigned int GetMaxConnectingPieces(EDGETYPE direction) const;
	const track_target_ptr & GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) const { return GetConnectingPiece(direction); }
	virtual bool Reservation(EDGETYPE direction, unsigned int index, unsigned int rr_flags, route *resroute);

	virtual std::string GetTypeName() const { return "Crossover"; }

	static std::string GetTypeSerialisationNameStatic() { return "crossover"; }
	virtual std::string GetTypeSerialisationName() const { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec);
	virtual void Serialise(serialiser_output &so, error_collection &ec) const;

	protected:
	bool HalfConnect(EDGETYPE this_entrance_direction, const track_target_ptr &target_entrance);
	virtual EDGETYPE GetAvailableAutoConnectionDirection(bool forwardconnection) const { return EDGE_NULL; }
	virtual void GetListOfEdges(std::vector<edgelistitem> &outputlist) const;
};

class doubleslip : public genericpoints {
	track_target_ptr frontleft;
	track_target_ptr frontright;
	track_target_ptr backright;
	track_target_ptr backleft;
	unsigned int pflags[4] = { 0, 0, 0, 0 };
	track_reservation_state trs;
	unsigned int dof = 2;
	unsigned int fail_pflags = PTF_INVALID;

	unsigned int dsflags;
	enum {
		DSF_NO_FL_BL	= 1<<0,
		DSF_NO_FR_BL	= 1<<1,
		DSF_NO_FL_BR	= 1<<2,
		DSF_NO_FR_BR	= 1<<3,
	};

	inline unsigned int GetCurrentPointIndex(EDGETYPE direction) const {
		switch(direction) {
			case EDGE_DS_FL:
				return 0;
			case EDGE_DS_FR:
				return 1;
			case EDGE_DS_BR:
				return 2;
			case EDGE_DS_BL:
				return 3;
			default:
				return UINT_MAX;
		}
	}

	private:
	inline const unsigned int &GetCurrentPointFlagsIntl(EDGETYPE direction) const {
		unsigned int index = GetCurrentPointIndex(direction);
		if(index < 4) return pflags[index];
		else return fail_pflags;
	}

	public:
	inline unsigned int &GetCurrentPointFlags(EDGETYPE direction) { return const_cast<unsigned int &>(GetCurrentPointFlagsIntl(direction)); }
	inline unsigned int GetCurrentPointFlags(EDGETYPE direction) const { return GetCurrentPointFlagsIntl(direction); }

	inline EDGETYPE GetConnectingPointDirection(EDGETYPE direction, bool reverse) const {
		switch(direction) {
			case EDGE_DS_FL:
				return reverse ? EDGE_DS_BL : EDGE_DS_BR;
			case EDGE_DS_FR:
				return reverse ? EDGE_DS_BR : EDGE_DS_BL;
			case EDGE_DS_BR:
				return reverse ? EDGE_DS_FR : EDGE_DS_FL;
			case EDGE_DS_BL:
				return reverse ? EDGE_DS_FL : EDGE_DS_FR;
			default:
				return EDGE_NULL;
		}
	}

	private:
	inline const track_target_ptr *GetInputPieceIntl(EDGETYPE direction) const {
		switch(direction) {
			case EDGE_DS_FL:
				return &frontleft;
			case EDGE_DS_FR:
				return &frontright;
			case EDGE_DS_BR:
				return &backright;
			case EDGE_DS_BL:
				return &backleft;
			default:
				return 0;
		}
	}
	
	public:
	inline track_target_ptr *GetInputPiece(EDGETYPE direction) { return const_cast<track_target_ptr *>(GetInputPieceIntl(direction)); }
	inline const track_target_ptr *GetInputPiece(EDGETYPE direction) const { return GetInputPieceIntl(direction); }
	
	inline const track_target_ptr &GetInputPieceOrEmpty(EDGETYPE direction) const {
		const track_target_ptr *piece = GetInputPiece(direction);
		if(piece) return *piece;
		else return empty_track_target;
	}

	public:
	doubleslip(world &w_) : genericpoints(w_), dsflags(0) { }

	const track_target_ptr & GetConnectingPiece(EDGETYPE direction) const;
	EDGETYPE GetReverseDirection(EDGETYPE direction) const;
	unsigned int GetFlags(EDGETYPE direction) const;
	virtual EDGETYPE GetDefaultValidDirecton() const { return EDGE_DS_FL; }

	unsigned int GetMaxConnectingPieces(EDGETYPE direction) const;
	const track_target_ptr & GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) const;
	virtual bool Reservation(EDGETYPE direction, unsigned int index, unsigned int rr_flags, route *resroute);
	virtual void ReservationActions(EDGETYPE direction, unsigned int index, unsigned int rr_flags, route *resroute, world &w, std::function<void(action &&reservation_act)> submitaction);

	virtual std::string GetTypeName() const { return "Double Slip Points"; }

	virtual unsigned int GetPointCount() const { return 4; }
	virtual unsigned int GetPointFlags(unsigned int points_index) const;
	virtual unsigned int SetPointFlagsMasked(unsigned int points_index, unsigned int set_flags, unsigned int mask_flags);

	static std::string GetTypeSerialisationNameStatic() { return "doubleslip"; }
	virtual std::string GetTypeSerialisationName() const { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec);
	virtual void Serialise(serialiser_output &so, error_collection &ec) const;

	protected:
	bool HalfConnect(EDGETYPE this_entrance_direction, const track_target_ptr &target_entrance);
	virtual EDGETYPE GetAvailableAutoConnectionDirection(bool forwardconnection) const { return EDGE_NULL; }
	virtual void GetListOfEdges(std::vector<edgelistitem> &outputlist) const;
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
std::ostream& operator<<(std::ostream& os, const track_target_ptr& obj);
std::ostream& operator<<(std::ostream& os, const track_location& obj);
std::ostream& operator<<(std::ostream& os, const EDGETYPE& obj);

const char * SerialiseDirectionName(const EDGETYPE& obj);
bool DeserialiseDirectionName(EDGETYPE& obj, const char *dirname);

#endif
