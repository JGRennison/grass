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
	void TrainEnter(EDGETYPE direction, train *t) override;
	void TrainLeave(EDGETYPE direction, train *t) override;
	virtual unsigned int GetPointCount() const = 0;
	virtual unsigned int GetPointFlags(unsigned int points_index) const = 0;
	virtual unsigned int SetPointFlagsMasked(unsigned int points_index, unsigned int set_flags, unsigned int mask_flags)  = 0;

	virtual std::string GetTypeName() const override { return "Generic Points"; }

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

	const track_target_ptr & GetConnectingPiece(EDGETYPE direction) const override;
	EDGETYPE GetReverseDirection(EDGETYPE direction) const override;
	unsigned int GetFlags(EDGETYPE direction) const override;
	virtual EDGETYPE GetDefaultValidDirecton() const override { return EDGE_PTS_FACE; }

	unsigned int GetMaxConnectingPieces(EDGETYPE direction) const override;
	const track_target_ptr & GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) const override;
	virtual bool Reservation(EDGETYPE direction, unsigned int index, unsigned int rr_flags, const route *resroute) override;
	virtual void ReservationActions(EDGETYPE direction, unsigned int index, unsigned int rr_flags, const route *resroute, std::function<void(action &&reservation_act)> submitaction) override;

	virtual std::string GetTypeName() const override { return "Points"; }

	virtual unsigned int GetPointCount() const override { return 1; }
	virtual unsigned int GetPointFlags(unsigned int points_index) const override;
	virtual unsigned int SetPointFlagsMasked(unsigned int points_index, unsigned int set_flags, unsigned int mask_flags) override;

	static std::string GetTypeSerialisationNameStatic() { return "points"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	protected:
	bool HalfConnect(EDGETYPE this_entrance_direction, const track_target_ptr &target_entrance) override;
	virtual EDGETYPE GetAvailableAutoConnectionDirection(bool forwardconnection) const override;
	virtual void GetListOfEdges(std::vector<edgelistitem> &outputlist) const override;
};

class catchpoints : public genericpoints {
	track_target_ptr prev;
	track_target_ptr next;
	unsigned int pflags;
	track_reservation_state trs;

	public:
	catchpoints(world &w_) : genericpoints(w_), pflags(PTF_REV) { }

	const track_target_ptr & GetConnectingPiece(EDGETYPE direction) const override;
	EDGETYPE GetReverseDirection(EDGETYPE direction) const override;
	unsigned int GetFlags(EDGETYPE direction) const override;
	virtual EDGETYPE GetDefaultValidDirecton() const override { return EDGE_PTS_FACE; }

	unsigned int GetMaxConnectingPieces(EDGETYPE direction) const override;
	const track_target_ptr & GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) const override;
	virtual bool Reservation(EDGETYPE direction, unsigned int index, unsigned int rr_flags, const route *resroute) override;
	virtual void ReservationActions(EDGETYPE direction, unsigned int index, unsigned int rr_flags, const route *resroute, std::function<void(action &&reservation_act)> submitaction) override;

	virtual std::string GetTypeName() const override { return "Catch Points"; }

	virtual unsigned int GetPointCount() const override { return 1; }
	virtual unsigned int GetPointFlags(unsigned int points_index) const override;
	virtual unsigned int SetPointFlagsMasked(unsigned int points_index, unsigned int set_flags, unsigned int mask_flags) override;

	static std::string GetTypeSerialisationNameStatic() { return "catchpoints"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	protected:
	bool HalfConnect(EDGETYPE this_entrance_direction, const track_target_ptr &target_entrance) override;
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
	void TrainEnter(EDGETYPE direction, train *t) override { }
	void TrainLeave(EDGETYPE direction, train *t) override { }

	const track_target_ptr & GetConnectingPiece(EDGETYPE direction) const override;
	EDGETYPE GetReverseDirection(EDGETYPE direction) const override;
	unsigned int GetFlags(EDGETYPE direction) const override;
	virtual EDGETYPE GetDefaultValidDirecton() const override { return EDGE_PTS_FACE; }

	unsigned int GetMaxConnectingPieces(EDGETYPE direction) const override;
	const track_target_ptr & GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) const override { return GetConnectingPiece(direction); }
	virtual bool Reservation(EDGETYPE direction, unsigned int index, unsigned int rr_flags, const route *resroute) override;

	virtual std::string GetTypeName() const override { return "Spring Points"; }

	static std::string GetTypeSerialisationNameStatic() { return "springpoints"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	bool GetSendReverseFlag() const { return sendreverse; }
	void SetSendReverseFlag(bool sr) { sendreverse = sr; }

	protected:
	bool HalfConnect(EDGETYPE this_entrance_direction, const track_target_ptr &target_entrance) override;
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
		DSF_NO_TRACK_MASK = DSF_NO_FL_BL | DSF_NO_FR_BL | DSF_NO_FL_BR | DSF_NO_FR_BR,
	};
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

	const track_target_ptr & GetConnectingPiece(EDGETYPE direction) const override;
	EDGETYPE GetReverseDirection(EDGETYPE direction) const override;
	unsigned int GetFlags(EDGETYPE direction) const override;
	virtual EDGETYPE GetDefaultValidDirecton() const override { return EDGE_DS_FL; }

	unsigned int GetMaxConnectingPieces(EDGETYPE direction) const override;
	const track_target_ptr & GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) const override;
	virtual bool Reservation(EDGETYPE direction, unsigned int index, unsigned int rr_flags, const route *resroute) override;
	virtual void ReservationActions(EDGETYPE direction, unsigned int index, unsigned int rr_flags, const route *resroute, std::function<void(action &&reservation_act)> submitaction) override;

	virtual std::string GetTypeName() const  override{ return "Double Slip Points"; }

	virtual unsigned int GetPointCount() const override { return 4; }
	virtual unsigned int GetPointFlags(unsigned int points_index) const override;
	virtual unsigned int SetPointFlagsMasked(unsigned int points_index, unsigned int set_flags, unsigned int mask_flags) override;

	static std::string GetTypeSerialisationNameStatic() { return "doubleslip"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	protected:
	bool HalfConnect(EDGETYPE this_entrance_direction, const track_target_ptr &target_entrance) override;
	virtual EDGETYPE GetAvailableAutoConnectionDirection(bool forwardconnection) const  override{ return EDGE_NULL; }
	virtual void GetListOfEdges(std::vector<edgelistitem> &outputlist) const override;
};

#endif