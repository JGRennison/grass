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

#ifndef INC_POINTS_ALREADY
#define INC_POINTS_ALREADY

#include "track.h"
#include "trackreservation.h"

class genericpoints : public genericzlentrack {
	protected:
	track_reservation_state trs;

	public:
	enum class PTF {
		ZERO		= 0,
		REV		= 1<<0,
		OOC		= 1<<1,
		LOCKED		= 1<<2,
		REMINDER	= 1<<3,
		FAILEDNORM	= 1<<4,
		FAILEDREV	= 1<<5,
		INVALID		= 1<<6,
		FIXED		= 1<<7,
		SERIALISABLE	= REV | OOC | LOCKED | REMINDER | FAILEDNORM | FAILEDREV,
	};

	genericpoints(world &w_) : genericzlentrack(w_) { }
	virtual void TrainEnter(EDGETYPE direction, train *t) override;
	virtual void TrainLeave(EDGETYPE direction, train *t) override;
	virtual unsigned int GetPointCount() const = 0;
	virtual PTF GetPointFlags(unsigned int points_index) const = 0;
	virtual PTF SetPointFlagsMasked(unsigned int points_index, PTF set_flags, PTF mask_flags)  = 0;

	virtual std::string GetTypeName() const override { return "Generic Points"; }

	inline bool IsFlagsOOC(PTF pflags) const;
	inline bool IsOOC(unsigned int points_index) const {
		return IsFlagsOOC(GetPointFlags(points_index));
	}

	inline bool IsFlagsImmovable(PTF pflags) const;
	inline bool IsImmovable(unsigned int points_index) const {
		return IsFlagsImmovable(GetPointFlags(points_index));
	}

	virtual unsigned int GetTRSList(std::vector<track_reservation_state *> &outputlist) override { outputlist.push_back(&trs); return 1; }
	virtual GTF GetFlags(EDGETYPE direction) const override;
};
template<> struct enum_traits< genericpoints::PTF > {	static constexpr bool flags = true; };

inline bool genericpoints::IsFlagsOOC(PTF pflags) const {
	if(pflags & PTF::OOC) return true;
	if(pflags & PTF::REV && pflags & PTF::FAILEDREV) return true;
	if(!(pflags & PTF::REV) && pflags & PTF::FAILEDNORM) return true;
	return false;
}
inline bool genericpoints::IsFlagsImmovable(PTF pflags) const {
	if(pflags & (PTF::LOCKED | PTF::REMINDER)) return true;
	return false;
}

class points : public genericpoints {
	track_target_ptr prev;
	track_target_ptr normal;
	track_target_ptr reverse;
	PTF pflags = PTF::ZERO;

	public:
	points(world &w_) : genericpoints(w_) { }

	virtual EDGETYPE GetReverseDirection(EDGETYPE direction) const override;
	virtual EDGETYPE GetDefaultValidDirecton() const override { return EDGE_PTS_FACE; }

	virtual const track_target_ptr & GetConnectingPiece(EDGETYPE direction) const override;
	virtual bool IsEdgeValid(EDGETYPE edge) const override;
	virtual const track_target_ptr & GetEdgeConnectingPiece(EDGETYPE edgeid) const override;
	unsigned int GetMaxConnectingPieces(EDGETYPE direction) const override;
	virtual const track_target_ptr & GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) const override;
	virtual bool Reservation(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute, std::string* failreasonkey = 0) override;
	virtual void ReservationActions(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute, std::function<void(action &&reservation_act)> submitaction) override;

	virtual std::string GetTypeName() const override { return "Points"; }

	virtual unsigned int GetPointCount() const override { return 1; }
	virtual PTF GetPointFlags(unsigned int points_index) const override;
	virtual PTF SetPointFlagsMasked(unsigned int points_index, PTF set_flags, PTF mask_flags) override;

	static std::string GetTypeSerialisationNameStatic() { return "points"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	virtual bool IsTrackAlwaysPassable() const override { return false; }

	protected:
	virtual EDGETYPE GetAvailableAutoConnectionDirection(bool forwardconnection) const override;
	virtual void GetListOfEdges(std::vector<edgelistitem> &outputlist) const override;
};

class catchpoints : public genericpoints {
	track_target_ptr prev;
	track_target_ptr next;
	PTF pflags;
	track_reservation_state trs;

	public:
	catchpoints(world &w_) : genericpoints(w_), pflags(PTF::REV) { }

	EDGETYPE GetReverseDirection(EDGETYPE direction) const override;
	virtual EDGETYPE GetDefaultValidDirecton() const override { return EDGE_PTS_FACE; }

	const track_target_ptr & GetConnectingPiece(EDGETYPE direction) const override;
	virtual bool IsEdgeValid(EDGETYPE edge) const override;
	virtual const track_target_ptr & GetEdgeConnectingPiece(EDGETYPE edgeid) const override;
	unsigned int GetMaxConnectingPieces(EDGETYPE direction) const override;
	const track_target_ptr & GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) const override;

	virtual bool Reservation(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute, std::string* failreasonkey = 0) override;
	virtual void ReservationActions(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute, std::function<void(action &&reservation_act)> submitaction) override;

	virtual std::string GetTypeName() const override { return "Catch Points"; }

	virtual unsigned int GetPointCount() const override { return 1; }
	virtual PTF GetPointFlags(unsigned int points_index) const override;
	virtual PTF SetPointFlagsMasked(unsigned int points_index, PTF set_flags, PTF mask_flags) override;

	static std::string GetTypeSerialisationNameStatic() { return "catchpoints"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	protected:
	virtual EDGETYPE GetAvailableAutoConnectionDirection(bool forwardconnection) const override;
	virtual void GetListOfEdges(std::vector<edgelistitem> &outputlist) const override;
};

class springpoints : public genericzlentrack {
	track_target_ptr prev;
	track_target_ptr normal;
	track_target_ptr reverse;
	bool sendreverse;
	track_reservation_state trs;

	public:
	springpoints(world &w_) : genericzlentrack(w_), sendreverse(false) { }
	virtual void TrainEnter(EDGETYPE direction, train *t) override { }
	virtual void TrainLeave(EDGETYPE direction, train *t) override { }

	virtual const track_target_ptr & GetConnectingPiece(EDGETYPE direction) const override;
	virtual EDGETYPE GetReverseDirection(EDGETYPE direction) const override;
	virtual GTF GetFlags(EDGETYPE direction) const override;
	virtual EDGETYPE GetDefaultValidDirecton() const override { return EDGE_PTS_FACE; }

	virtual bool IsEdgeValid(EDGETYPE edge) const override;
	virtual const track_target_ptr & GetEdgeConnectingPiece(EDGETYPE edgeid) const override;
	virtual unsigned int GetMaxConnectingPieces(EDGETYPE direction) const override;
	virtual const track_target_ptr & GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) const override { return GetConnectingPiece(direction); }
	virtual bool Reservation(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute, std::string* failreasonkey) override;

	virtual std::string GetTypeName() const override { return "Spring Points"; }

	static std::string GetTypeSerialisationNameStatic() { return "springpoints"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	bool GetSendReverseFlag() const { return sendreverse; }
	void SetSendReverseFlag(bool sr) { sendreverse = sr; }

	protected:
	virtual EDGETYPE GetAvailableAutoConnectionDirection(bool forwardconnection) const override;
	virtual void GetListOfEdges(std::vector<edgelistitem> &outputlist) const override;
};

class test_doubleslip;

class doubleslip : public genericpoints {
	friend test_doubleslip;

	track_target_ptr frontleft;
	track_target_ptr frontright;
	track_target_ptr backright;
	track_target_ptr backleft;
	PTF pflags[4] = { PTF::ZERO, PTF::ZERO, PTF::ZERO, PTF::ZERO };
	track_reservation_state trs;
	unsigned int dof = 2;
	PTF fail_pflags = PTF::INVALID;

	enum class DSF {
		ZERO		= 0,
		NO_FL_BL	= 1<<0,
		NO_FR_BL	= 1<<1,
		NO_FL_BR	= 1<<2,
		NO_FR_BR	= 1<<3,
		NO_TRACK_MASK = NO_FL_BL | NO_FR_BL | NO_FL_BR | NO_FR_BR,
	};
	DSF dsflags;
	void UpdatePointsFixedStatesFromMissingTrackEdges();

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
	inline const PTF &GetCurrentPointFlagsIntl(EDGETYPE direction) const {
		unsigned int index = GetCurrentPointIndex(direction);
		if(index < 4) return pflags[index];
		else return fail_pflags;
	}

	public:
	inline PTF &GetCurrentPointFlags(EDGETYPE direction) { return const_cast<PTF &>(GetCurrentPointFlagsIntl(direction)); }
	inline PTF GetCurrentPointFlags(EDGETYPE direction) const { return GetCurrentPointFlagsIntl(direction); }

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
	doubleslip(world &w_) : genericpoints(w_), dsflags(DSF::ZERO) { }

	virtual const track_target_ptr & GetConnectingPiece(EDGETYPE direction) const override;
	virtual EDGETYPE GetReverseDirection(EDGETYPE direction) const override;
	virtual EDGETYPE GetDefaultValidDirecton() const override { return EDGE_DS_FL; }

	virtual bool IsEdgeValid(EDGETYPE edge) const override;
	virtual const track_target_ptr & GetEdgeConnectingPiece(EDGETYPE edgeid) const override;
	virtual unsigned int GetMaxConnectingPieces(EDGETYPE direction) const override;
	virtual const track_target_ptr & GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) const override;
	virtual bool Reservation(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute, std::string* failreasonkey = 0) override;
	virtual void ReservationActions(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute, std::function<void(action &&reservation_act)> submitaction) override;

	virtual std::string GetTypeName() const  override{ return "Double Slip Points"; }

	virtual unsigned int GetPointCount() const override { return 4; }
	virtual PTF GetPointFlags(unsigned int points_index) const override;
	virtual PTF SetPointFlagsMasked(unsigned int points_index, PTF set_flags, PTF mask_flags) override;

	static std::string GetTypeSerialisationNameStatic() { return "doubleslip"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	protected:
	virtual EDGETYPE GetAvailableAutoConnectionDirection(bool forwardconnection) const  override{ return EDGE_NULL; }
	virtual void GetListOfEdges(std::vector<edgelistitem> &outputlist) const override;
};

template<> struct enum_traits< doubleslip::DSF > {	static constexpr bool flags = true; };

#endif
