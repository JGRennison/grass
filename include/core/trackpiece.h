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

#ifndef INC_TRACKPIECE_ALREADY
#define INC_TRACKPIECE_ALREADY

#include "core/track.h"
#include "core/trackreservation.h"

class trackseg : public generictrack {
	unsigned int length;
	int elevationdelta;
	speedrestrictionset speed_limits;
	tractionset tractiontypes;
	track_circuit *tc;
	std::vector<track_train_counter_block *> ttcbs;
	unsigned int traincount;
	std::vector<train *> occupying_trains;
	track_target_ptr next;
	track_target_ptr prev;
	track_reservation_state trs;

	public:
	trackseg(world &w_) : generictrack(w_), length(0), elevationdelta(0), tc(0), traincount(0) { }
	void TrainEnter(EDGETYPE direction, train *t) override;
	void TrainLeave(EDGETYPE direction, train *t) override;
	virtual const track_target_ptr & GetConnectingPiece(EDGETYPE direction) const override;
	virtual unsigned int GetStartOffset(EDGETYPE direction) const override;
	virtual int GetElevationDelta(EDGETYPE direction) const override;
	virtual unsigned int GetLength(EDGETYPE direction) const override;
	virtual const speedrestrictionset *GetSpeedRestrictions() const override;
	virtual const tractionset *GetTractionTypes() const override;
	virtual unsigned int GetNewOffset(EDGETYPE direction, unsigned int currentoffset, unsigned int step) const override;
	virtual unsigned int GetRemainingLength(EDGETYPE direction, unsigned int currentoffset) const override;
	virtual track_circuit *GetTrackCircuit() const override;
	virtual const std::vector<track_train_counter_block *> *GetOtherTrackTriggers() const override;
	virtual GTF GetFlags(EDGETYPE direction) const override;
	virtual EDGETYPE GetReverseDirection(EDGETYPE direction) const override;
	virtual EDGETYPE GetDefaultValidDirecton() const override { return EDGE_FRONT; }

	virtual bool IsEdgeValid(EDGETYPE edge) const override;
	virtual const track_target_ptr & GetEdgeConnectingPiece(EDGETYPE edgeid) const override;
	virtual unsigned int GetMaxConnectingPieces(EDGETYPE direction) const;
	virtual const track_target_ptr & GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) const override;
	virtual bool ReservationV(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute, std::string* failreasonkey) override;

	virtual std::string GetTypeName() const { return "Track Segment"; }

	virtual trackseg & SetLength(unsigned int length);
	virtual trackseg & AddSpeedRestriction(speed_restriction sr);
	virtual trackseg & SetElevationDelta(unsigned int elevationdelta);
	virtual trackseg & SetTrackCircuit(track_circuit *tc);

	static std::string GetTypeSerialisationNameStatic() { return "trackseg"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	protected:
	virtual unsigned int GetTRSList(std::vector<track_reservation_state *> &outputlist) override { outputlist.push_back(&trs); return 1; }
	virtual bool CanHaveBerth() const override { return true; }

	virtual EDGETYPE GetAvailableAutoConnectionDirection(bool forwardconnection) const override;
	virtual void GetListOfEdges(std::vector<edgelistitem> &outputlist) const override;

	public:
	virtual void GetTrainOccupationState(std::vector<train_occupation> &train_os) override;
};

class crossover : public genericzlentrack {
	track_target_ptr left;
	track_target_ptr right;
	track_target_ptr front;
	track_target_ptr back;
	track_reservation_state trs;

	public:
	crossover(world &w_) : genericzlentrack(w_) { }
	virtual void TrainEnter(EDGETYPE direction, train *t) override { }
	virtual void TrainLeave(EDGETYPE direction, train *t) override { }

	virtual const track_target_ptr & GetConnectingPiece(EDGETYPE direction) const override;
	virtual EDGETYPE GetReverseDirection(EDGETYPE direction) const override;
	virtual GTF GetFlags(EDGETYPE direction) const override;
	virtual EDGETYPE GetDefaultValidDirecton() const override { return EDGE_X_LEFT; }

	virtual bool IsEdgeValid(EDGETYPE edge) const override;
	virtual const track_target_ptr & GetEdgeConnectingPiece(EDGETYPE edgeid) const override;
	virtual unsigned int GetMaxConnectingPieces(EDGETYPE direction) const override;
	virtual const track_target_ptr & GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) const override { return GetConnectingPiece(direction); }
	virtual bool ReservationV(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute, std::string* failreasonkey) override;

	virtual std::string GetTypeName() const override { return "Crossover"; }

	static std::string GetTypeSerialisationNameStatic() { return "crossover"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	virtual unsigned int GetTRSList(std::vector<track_reservation_state *> &outputlist) override { outputlist.push_back(&trs); return 1; }

	protected:
	virtual EDGETYPE GetAvailableAutoConnectionDirection(bool forwardconnection) const override;
	virtual void GetListOfEdges(std::vector<edgelistitem> &outputlist) const override;
};

#endif


