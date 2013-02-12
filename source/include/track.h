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

#include "trackcircuit.h"
#include "tractiontype.h"
#include "error.h"

struct speed_class;
class train;
class generictrack;
template <typename T> struct vartrack_target_ptr;
typedef vartrack_target_ptr<generictrack> track_target_ptr;
class track_location;

struct speed_restriction {
	unsigned int speed;
	speed_class *speedclass;
};

class speedrestrictionset {
	std::forward_list<speed_restriction> speeds;

	public:
	unsigned int GetTrainTrackSpeedLimit(train *t) const;
	inline void AddSpeedRestriction(const speed_restriction &sr) {
		speeds.push_front(sr);
	}
};

typedef enum {
	TDIR_NULL = 0,
	TDIR_FORWARD,		//forward direction on track
	TDIR_REVERSE,		//reverse direction on track

	TDIR_PTS_FACE,		//points: facing direction
	TDIR_PTS_NORMAL,	//points: normal trailing direction
	TDIR_PTS_REVERSE,	//points: reverse trailing direction
} DIRTYPE;

class tractionset {
	std::forward_list<traction_type> tractions;
};

class generictrack {
	std::string name;
	public:
	virtual const speedrestrictionset *GetSpeedRestrictions() const;
	virtual const tractionset *GetTractionTypes() const;
	virtual const track_target_ptr & GetConnectingPiece(DIRTYPE direction) const = 0;
	virtual unsigned int GetNewOffset(DIRTYPE direction, unsigned int currentoffset, unsigned int step) const = 0;
	virtual unsigned int GetStartOffset(DIRTYPE direction) const = 0;
	virtual unsigned int GetRemainingLength(DIRTYPE direction, unsigned int currentoffset) const = 0;
	virtual unsigned int GetLength(DIRTYPE direction) const = 0;
	virtual int GetElevationDelta(DIRTYPE direction) const = 0;
	virtual int GetPartialElevationDelta(DIRTYPE direction, unsigned int displacement) const;
	virtual track_circuit *GetTrackCircuit() const;
	virtual unsigned int GetFlags(DIRTYPE direction) const;
	virtual void TrainEnter(DIRTYPE direction, train *t) = 0;
	virtual void TrainLeave(DIRTYPE direction, train *t) = 0;
	virtual DIRTYPE GetReverseDirection(DIRTYPE direction) const = 0;

	virtual unsigned int GetMaxConnectingPieces(DIRTYPE direction) const = 0;
	virtual const track_target_ptr & GetConnectingPieceByIndex(DIRTYPE direction, unsigned int index) const = 0;

	bool FullConnect(DIRTYPE this_entrance_direction, const track_target_ptr &target_entrance, error_collection &ec);

	virtual std::string GetTypeName() const { return "Generic Track"; }

	virtual std::string GetName() const { return name; }
	virtual void SetName(std::string newname) { name = newname; }
	virtual std::string GetFriendlyName() const;

	virtual generictrack & SetLength(unsigned int length);
	virtual generictrack & AddSpeedRestriction(speed_restriction sr);
	virtual generictrack & SetElevationDelta(unsigned int elevationdelta);
	virtual generictrack & SetTrackCircuit(track_circuit *tc);

	virtual bool PostLayoutInit(error_collection &ec) { return true; }	//return false to discontinue initing piece

	enum {
		GTF_ROUTESET	= 1<<0,
		GTF_ROUTETHISDIR= 1<<1,
		GTF_ROUTEFORK	= 1<<2,
		GTF_ROUTINGPOINT= 1<<3,
	};

	protected:
	virtual bool HalfConnect(DIRTYPE this_entrance_direction, const track_target_ptr &target_entrance) = 0;
	static bool TryConnectPiece(track_target_ptr &piece_var, const track_target_ptr &new_target);
};

template <typename T> struct vartrack_target_ptr {
	T *track;
	DIRTYPE direction;
	inline bool IsValid() const { return track; }
	inline void ReverseDirection() {
		direction = track->GetReverseDirection(direction);
	}
	inline void Reset() {
		track = 0;
		direction = TDIR_NULL;
	}
	vartrack_target_ptr() { Reset(); }
	vartrack_target_ptr(T *track_, DIRTYPE direction_) : track(track_), direction(direction_) { }
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
	inline void SetTargetStartLocation(generictrack *t, DIRTYPE dir) {
		SetTargetStartLocation(track_target_ptr(t, dir));
	}
	inline const DIRTYPE &GetDirection() const {
		return trackpiece.direction;
	}
	inline generictrack * const &GetTrack() const  {
		return trackpiece.track;
	}
	inline const unsigned int &GetOffset() const  {
		return offset;
	}
	inline DIRTYPE &GetDirection() {
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
	track_location(generictrack *track_, DIRTYPE direction_, unsigned int offset_) : trackpiece(track_, direction_), offset(offset_) { }
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

	public:
	trackseg() : length(0), elevationdelta(0), tc(0), traincount(0) { }
	void TrainEnter(DIRTYPE direction, train *t);
	void TrainLeave(DIRTYPE direction, train *t);
	const track_target_ptr & GetConnectingPiece(DIRTYPE direction) const;
	unsigned int GetStartOffset(DIRTYPE direction) const;
	int GetElevationDelta(DIRTYPE direction) const;
	unsigned int GetLength(DIRTYPE direction) const;
	const speedrestrictionset *GetSpeedRestrictions() const;
	const tractionset *GetTractionTypes() const;
	unsigned int GetNewOffset(DIRTYPE direction, unsigned int currentoffset, unsigned int step) const;
	unsigned int GetRemainingLength(DIRTYPE direction, unsigned int currentoffset) const;
	track_circuit *GetTrackCircuit() const;
	unsigned int GetFlags(DIRTYPE direction) const;
	DIRTYPE GetReverseDirection(DIRTYPE direction) const;

	unsigned int GetMaxConnectingPieces(DIRTYPE direction) const;
	const track_target_ptr & GetConnectingPieceByIndex(DIRTYPE direction, unsigned int index) const;

	virtual std::string GetTypeName() const { return "Track Segment"; }

	trackseg & SetLength(unsigned int length);
	trackseg & AddSpeedRestriction(speed_restriction sr);
	trackseg & SetElevationDelta(unsigned int elevationdelta);
	trackseg & SetTrackCircuit(track_circuit *tc);

	protected:
	bool HalfConnect(DIRTYPE this_entrance_direction, const track_target_ptr &target_entrance);
};

class genericzlentrack : public generictrack {
	public:
	unsigned int GetNewOffset(DIRTYPE direction, unsigned int currentoffset, unsigned int step) const;
	unsigned int GetRemainingLength(DIRTYPE direction, unsigned int currentoffset) const;
	unsigned int GetLength(DIRTYPE direction) const;
	unsigned int GetStartOffset(DIRTYPE direction) const;
	int GetElevationDelta(DIRTYPE direction) const;
};

class genericpoints : public genericzlentrack {
	public:
	void TrainEnter(DIRTYPE direction, train *t);
	void TrainLeave(DIRTYPE direction, train *t);

	virtual std::string GetTypeName() const { return "Generic Points"; }
};

class points : public genericpoints {
	track_target_ptr prev;
	track_target_ptr normal;
	track_target_ptr reverse;
	unsigned int pflags;

	public:
	enum {
		PTF_REV		= 1<<0,
		PTF_OOC		= 1<<1,
		PTF_LOCKED	= 1<<2,
		PTF_REMINDER	= 1<<3,
		PTF_FAILED	= 1<<4,
	};

	points() : pflags(0) { }

	const track_target_ptr & GetConnectingPiece(DIRTYPE direction) const;
	DIRTYPE GetReverseDirection(DIRTYPE direction) const;
	unsigned int GetFlags(DIRTYPE direction) const;

	unsigned int GetMaxConnectingPieces(DIRTYPE direction) const;
	const track_target_ptr & GetConnectingPieceByIndex(DIRTYPE direction, unsigned int index) const;

	virtual std::string GetTypeName() const { return "Points"; }

	virtual unsigned int GetPointFlags() const;
	virtual unsigned int SetPointFlagsMasked(unsigned int set_flags, unsigned int mask_flags);

	protected:
	bool HalfConnect(DIRTYPE this_entrance_direction, const track_target_ptr &target_entrance);
};

std::ostream& operator<<(std::ostream& os, const track_target_ptr& obj);
std::ostream& operator<<(std::ostream& os, const track_location& obj);
std::ostream& operator<<(std::ostream& os, const DIRTYPE& obj);

#endif
