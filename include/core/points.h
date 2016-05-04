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

#ifndef INC_POINTS_ALREADY
#define INC_POINTS_ALREADY

#include "core/track.h"
#include "core/track_reservation.h"

class generic_points : public generic_zlen_track {
	protected:
	track_reservation_state trs;

	public:
	enum class PTF {
		ZERO             = 0,
		REV              = 1<<0,
		OOC              = 1<<1,
		LOCKED           = 1<<2,
		REMINDER         = 1<<3,
		FAILED_NORM      = 1<<4,
		FAILED_REV       = 1<<5,
		INVALID          = 1<<6,
		FIXED            = 1<<7,
		COUPLED          = 1<<8,
		AUTO_NORMALISE   = 1<<9,
		SERIALISABLE     = REV | OOC | LOCKED | REMINDER | FAILED_NORM | FAILED_REV,
		ALL              = SERIALISABLE | INVALID | FIXED | COUPLED | AUTO_NORMALISE,
	};

	protected:
	PTF fail_pflags = PTF::INVALID;

	public:
	generic_points(world &w_) : generic_zlen_track(w_) { }
	virtual void TrainEnter(EDGE direction, train *t) override;
	virtual void TrainLeave(EDGE direction, train *t) override;
	virtual unsigned int GetPointsCount() const = 0;
	virtual const PTF &GetPointsFlagsRef(unsigned int points_index) const = 0;
	inline PTF &GetPointsFlagsRef(unsigned int points_index) { return const_cast<PTF &>(const_cast<const generic_points*>(this)->GetPointsFlagsRef(points_index)); }
	inline PTF GetPointsFlags(unsigned int points_index) const { return GetPointsFlagsRef(points_index); }
	PTF SetPointsFlagsMasked(unsigned int points_index, PTF set_flags, PTF mask_flags);
	virtual unsigned int GetPointsIndexByEdge(EDGE direction) const { return 0; }
	virtual bool IsCoupleable(EDGE direction) const { return false; }
	struct points_coupling {
		PTF *pflags;
		PTF xormask;
		generic_points *targ;
		unsigned int index;
		points_coupling(PTF *p, PTF x, generic_points *t, unsigned int i) : pflags(p), xormask(x), targ(t), index(i) { }
	};
	virtual void GetCouplingPointsFlagsByEdge(EDGE direction, std::vector<points_coupling> &output) { }
	virtual void CouplePointsFlagsAtIndexTo(unsigned int index, const points_coupling &pc) { }
	virtual std::vector<points_coupling> *GetCouplingVector(unsigned int index) { return nullptr; }

	// this is only called if PTF::AUTO_NORMALISE is set
	virtual bool ShouldAutoNormalise(unsigned int index, PTF change_flags) const;

	virtual bool PostLayoutInit(error_collection &ec) override;

	protected:
	void CommonReservationAction(unsigned int points_index, const reservation_request_action &req);

	public:
	virtual std::string GetTypeName() const override { return "Generic Points"; }

	static inline bool IsFlagsOOC(PTF pflags);
	inline bool IsOOC(unsigned int points_index) const {
		return IsFlagsOOC(GetPointsFlags(points_index));
	}

	static inline bool IsFlagsImmovable(PTF pflags);
	inline bool IsImmovable(unsigned int points_index) const {
		return IsFlagsImmovable(GetPointsFlags(points_index));
	}

	virtual unsigned int GetTRSList(std::vector<track_reservation_state *> &output_list) override { output_list.push_back(&trs); return 1; }
	virtual GTF GetFlags(EDGE direction) const override;
};
template<> struct enum_traits< generic_points::PTF > { static constexpr bool flags = true; };

inline bool generic_points::IsFlagsOOC(PTF pflags) {
	if (pflags & PTF::OOC) return true;
	if (pflags & PTF::REV && pflags & PTF::FAILED_REV) return true;
	if (!(pflags & PTF::REV) && pflags & PTF::FAILED_NORM) return true;
	return false;
}
inline bool generic_points::IsFlagsImmovable(PTF pflags) {
	if (pflags & (PTF::LOCKED | PTF::REMINDER)) return true;
	return false;
}

class points : public generic_points {
	track_target_ptr prev;
	track_target_ptr normal;
	track_target_ptr reverse;
	PTF pflags = PTF::ZERO;
	std::vector<points_coupling> couplings;

	void InitSightingDistances();

	public:
	points(world &w_) : generic_points(w_) { InitSightingDistances(); }

	virtual EDGE GetReverseDirection(EDGE direction) const override;
	virtual EDGE GetDefaultValidDirecton() const override { return EDGE::PTS_FACE; }

	virtual const edge_track_target GetConnectingPiece(EDGE direction) override;
	virtual bool IsEdgeValid(EDGE edge) const override;
	virtual const edge_track_target GetEdgeConnectingPiece(EDGE edgeid) override;
	unsigned int GetMaxConnectingPieces(EDGE direction) const override;
	virtual const edge_track_target GetConnectingPieceByIndex(EDGE direction, unsigned int index) override;
	virtual unsigned int GetCurrentNominalConnectionIndex(EDGE direction) const override;

	virtual std::string GetTypeName() const override { return "Points"; }

	virtual unsigned int GetPointsCount() const override { return 1; }
	virtual const PTF &GetPointsFlagsRef(unsigned int points_index) const override;

	static std::string GetTypeSerialisationNameStatic() { return "points"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	virtual bool IsTrackAlwaysPassable() const override { return false; }

	virtual bool IsCoupleable(EDGE direction) const override;
	virtual void GetCouplingPointsFlagsByEdge(EDGE direction, std::vector<points_coupling> &output) override;
	virtual void CouplePointsFlagsAtIndexTo(unsigned int index, const points_coupling &pc) override;
	virtual std::vector<points_coupling> *GetCouplingVector(unsigned int index) override { return &couplings; }

	protected:
	virtual EDGE GetAvailableAutoConnectionDirection(bool forward_connection) const override;
	virtual void GetListOfEdges(std::vector<edgelistitem> &output_list) const override;
	virtual reservation_result ReservationV(const reservation_request_res &req) override;
	virtual void ReservationActionsV(const reservation_request_action &req) override;
};

class catchpoints : public generic_points {
	track_target_ptr prev;
	track_target_ptr next;
	PTF pflags;

	void InitSightingDistances();

	public:
	catchpoints(world &w_) : generic_points(w_), pflags(PTF::AUTO_NORMALISE) { InitSightingDistances(); }

	EDGE GetReverseDirection(EDGE direction) const override;
	virtual EDGE GetDefaultValidDirecton() const override { return EDGE::FRONT; }

	const edge_track_target GetConnectingPiece(EDGE direction) override;
	virtual bool IsEdgeValid(EDGE edge) const override;
	virtual const edge_track_target GetEdgeConnectingPiece(EDGE edgeid) override;
	unsigned int GetMaxConnectingPieces(EDGE direction) const override;
	const edge_track_target GetConnectingPieceByIndex(EDGE direction, unsigned int index) override;

	virtual std::string GetTypeName() const override { return "Catch Points"; }

	virtual unsigned int GetPointsCount() const override { return 1; }
	virtual const PTF &GetPointsFlagsRef(unsigned int points_index) const override;

	static std::string GetTypeSerialisationNameStatic() { return "catchpoints"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	protected:
	virtual EDGE GetAvailableAutoConnectionDirection(bool forward_connection) const override;
	virtual void GetListOfEdges(std::vector<edgelistitem> &output_list) const override;
	virtual reservation_result ReservationV(const reservation_request_res &req) override;
	virtual void ReservationActionsV(const reservation_request_action &req) override;
};

class spring_points : public generic_zlen_track {
	track_target_ptr prev;
	track_target_ptr normal;
	track_target_ptr reverse;
	bool sendreverse;
	track_reservation_state trs;

	public:
	spring_points(world &w_) : generic_zlen_track(w_), sendreverse(false) { }
	virtual void TrainEnter(EDGE direction, train *t) override { }
	virtual void TrainLeave(EDGE direction, train *t) override { }

	virtual const edge_track_target GetConnectingPiece(EDGE direction) override;
	virtual EDGE GetReverseDirection(EDGE direction) const override;
	virtual GTF GetFlags(EDGE direction) const override;
	virtual EDGE GetDefaultValidDirecton() const override { return EDGE::PTS_FACE; }

	virtual bool IsEdgeValid(EDGE edge) const override;
	virtual const edge_track_target GetEdgeConnectingPiece(EDGE edgeid) override;
	virtual unsigned int GetMaxConnectingPieces(EDGE direction) const override;
	virtual const edge_track_target GetConnectingPieceByIndex(EDGE direction, unsigned int index) override { return GetConnectingPiece(direction); }

	virtual std::string GetTypeName() const override { return "Spring Points"; }

	static std::string GetTypeSerialisationNameStatic() { return "spring_points"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	bool GetSendReverseFlag() const { return sendreverse; }
	void SetSendReverseFlag(bool sr) { sendreverse = sr; }

	protected:
	virtual unsigned int GetTRSList(std::vector<track_reservation_state *> &output_list) override {
		output_list.push_back(&trs);
		return 1;
	}
	virtual EDGE GetAvailableAutoConnectionDirection(bool forward_connection) const override;
	virtual void GetListOfEdges(std::vector<edgelistitem> &output_list) const override;
	virtual reservation_result ReservationV(const reservation_request_res &req) override;
};

class test_double_slip;

class double_slip : public generic_points {
	friend test_double_slip;

	track_target_ptr frontleft;
	track_target_ptr frontright;
	track_target_ptr backright;
	track_target_ptr backleft;
	PTF pflags[4] = { PTF::ZERO, PTF::ZERO, PTF::ZERO, PTF::ZERO };
	std::vector<points_coupling> couplings[4];
	unsigned int dof = 2;

	enum class DSF {
		ZERO               = 0,
		NO_FL_BL           = 1<<0,
		NO_FR_BL           = 1<<1,
		NO_FL_BR           = 1<<2,
		NO_FR_BR           = 1<<3,
		NO_TRACK_MASK      = NO_FL_BL | NO_FR_BL | NO_FL_BR | NO_FR_BR,
	};
	DSF dsflags;
	void UpdatePointsFixedStatesFromMissingTrackEdges();
	void UpdateInternalCoupling();

	void InitSightingDistances();

	private:
	inline unsigned int GetPointsIndexByEdgeIntl(EDGE direction) const {
		switch (direction) {
			case EDGE::DS_FL:
				return 0;

			case EDGE::DS_FR:
				return 1;

			case EDGE::DS_BR:
				return 2;

			case EDGE::DS_BL:
				return 3;

			default:
				return UINT_MAX;
		}
	}

	inline const PTF &GetCurrentPointFlagsIntl(EDGE direction) const {
		unsigned int index = GetPointsIndexByEdgeIntl(direction);
		if (index < 4) {
			return pflags[index];
		} else {
			return fail_pflags;
		}
	}

	public:
	inline PTF &GetCurrentPointFlags(EDGE direction) { return const_cast<PTF &>(GetCurrentPointFlagsIntl(direction)); }
	inline PTF GetCurrentPointFlags(EDGE direction) const { return GetCurrentPointFlagsIntl(direction); }
	virtual unsigned int GetPointsIndexByEdge(EDGE direction) const override { return GetPointsIndexByEdgeIntl(direction); }

	inline EDGE GetConnectingPointDirection(EDGE direction, bool reverse) const {
		switch (direction) {
			case EDGE::DS_FL:
				return reverse ? EDGE::DS_BL : EDGE::DS_BR;

			case EDGE::DS_FR:
				return reverse ? EDGE::DS_BR : EDGE::DS_BL;

			case EDGE::DS_BR:
				return reverse ? EDGE::DS_FR : EDGE::DS_FL;

			case EDGE::DS_BL:
				return reverse ? EDGE::DS_FL : EDGE::DS_FR;

			default:
				return EDGE::INVALID;
		}
	}

	private:
	inline const track_target_ptr *GetInputPieceIntl(EDGE direction) const {
		switch (direction) {
			case EDGE::DS_FL:
				return &frontleft;

			case EDGE::DS_FR:
				return &frontright;

			case EDGE::DS_BR:
				return &backright;

			case EDGE::DS_BL:
				return &backleft;

			default:
				return nullptr;
		}
	}

	public:
	inline track_target_ptr *GetInputPiece(EDGE direction) { return const_cast<track_target_ptr *>(GetInputPieceIntl(direction)); }

	inline generic_track::edge_track_target GetInputPieceOrEmpty(EDGE direction) {
		track_target_ptr *piece = GetInputPiece(direction);
		if (piece) {
			return *piece;
		} else {
			return empty_track_target;
		}
	}

	public:
	double_slip(world &w_) : generic_points(w_), dsflags(DSF::ZERO) { InitSightingDistances(); }

	virtual const edge_track_target GetConnectingPiece(EDGE direction) override;
	virtual EDGE GetReverseDirection(EDGE direction) const override;
	virtual EDGE GetDefaultValidDirecton() const override { return EDGE::DS_FL; }

	virtual bool IsEdgeValid(EDGE edge) const override;
	virtual const edge_track_target GetEdgeConnectingPiece(EDGE edgeid) override;
	virtual unsigned int GetMaxConnectingPieces(EDGE direction) const override;
	virtual const edge_track_target GetConnectingPieceByIndex(EDGE direction, unsigned int index) override;
	virtual unsigned int GetCurrentNominalConnectionIndex(EDGE direction) const override;
	virtual std::string GetTypeName() const  override{ return "Double Slip Points"; }

	virtual unsigned int GetPointsCount() const override { return 4; }
	virtual const PTF &GetPointsFlagsRef(unsigned int points_index) const override;

	static std::string GetTypeSerialisationNameStatic() { return "double_slip"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	virtual bool IsCoupleable(EDGE direction) const override;
	virtual void GetCouplingPointsFlagsByEdge(EDGE direction, std::vector<points_coupling> &output) override;
	virtual void CouplePointsFlagsAtIndexTo(unsigned int index, const points_coupling &pc) override;
	virtual std::vector<points_coupling> *GetCouplingVector(unsigned int index) override { return &couplings[index]; }

	protected:
	virtual EDGE GetAvailableAutoConnectionDirection(bool forward_connection) const  override{ return EDGE::INVALID; }
	virtual void GetListOfEdges(std::vector<edgelistitem> &output_list) const override;
	virtual reservation_result ReservationV(const reservation_request_res &req) override;
	virtual void ReservationActionsV(const reservation_request_action &req) override;
};

template<> struct enum_traits< double_slip::DSF > { static constexpr bool flags = true; };

void DeserialisePointsCoupling(const deserialiser_input &di, error_collection &ec);

#endif
