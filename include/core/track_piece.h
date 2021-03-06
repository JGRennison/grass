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

#ifndef INC_TRACK_PIECE_ALREADY
#define INC_TRACK_PIECE_ALREADY

#include "core/track.h"
#include "core/track_reservation.h"

class track_seg : public generic_track {
	unsigned int length = 0;
	int elevation_delta = 0;
	speed_restriction_set speed_limits;
	traction_set traction_types;
	std::vector<track_train_counter_block *> ttcbs;
	unsigned int train_count = 0;
	std::vector<train *> occupying_trains;
	track_target_ptr next;
	track_target_ptr prev;
	track_reservation_state trs;

	public:
	track_seg(world &w_) : generic_track(w_) { }
	void TrainEnter(EDGE direction, train *t) override;
	void TrainLeave(EDGE direction, train *t) override;
	virtual const edge_track_target GetConnectingPiece(EDGE direction) override;
	virtual unsigned int GetStartOffset(EDGE direction) const override;
	virtual int GetElevationDelta(EDGE direction) const override;
	virtual unsigned int GetLength(EDGE direction) const override;
	virtual const speed_restriction_set *GetSpeedRestrictions() const override;
	virtual const traction_set *GetTractionTypes() const override;
	virtual unsigned int GetNewOffset(EDGE direction, unsigned int current_offset, unsigned int step) const override;
	virtual unsigned int GetRemainingLength(EDGE direction, unsigned int current_offset) const override;
	virtual const std::vector<track_train_counter_block *> *GetOtherTrackTriggers() const override;
	virtual GTF GetFlags(EDGE direction) const override;
	virtual EDGE GetReverseDirection(EDGE direction) const override;
	virtual EDGE GetDefaultValidDirecton() const override { return EDGE::FRONT; }

	virtual bool IsEdgeValid(EDGE edge) const override;
	virtual const edge_track_target GetEdgeConnectingPiece(EDGE edgeid) override;
	virtual unsigned int GetMaxConnectingPieces(EDGE direction) const;
	virtual const edge_track_target GetConnectingPieceByIndex(EDGE direction, unsigned int index) override;
	virtual reservation_result ReservationV(const reservation_request_res &req) override;

	virtual std::string GetTypeName() const { return "Track Segment"; }

	static std::string GetTypeSerialisationNameStatic() { return "track_seg"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	protected:
	virtual unsigned int GetTRSList(std::vector<track_reservation_state *> &output_list) override {
		output_list.push_back(&trs);
		return 1;
	}
	virtual bool CanHaveBerth() const override { return true; }

	virtual EDGE GetAvailableAutoConnectionDirection(bool forward_connection) const override;
	virtual void GetListOfEdges(std::vector<edgelistitem> &output_list) const override;

	public:
	virtual void GetTrainOccupationState(std::vector<train_occupation> &train_os) override;

	// For use of test code **only**
	virtual track_seg & SetLength(unsigned int length) override;
	virtual track_seg & SetElevationDelta(unsigned int elevation_delta) override;
};

class crossover : public generic_zlen_track {
	track_target_ptr left;
	track_target_ptr right;
	track_target_ptr front;
	track_target_ptr back;
	track_reservation_state trs;

	public:
	crossover(world &w_) : generic_zlen_track(w_) { }
	virtual void TrainEnter(EDGE direction, train *t) override { }
	virtual void TrainLeave(EDGE direction, train *t) override { }

	virtual const edge_track_target GetConnectingPiece(EDGE direction) override;
	virtual EDGE GetReverseDirection(EDGE direction) const override;
	virtual GTF GetFlags(EDGE direction) const override;
	virtual EDGE GetDefaultValidDirecton() const override { return EDGE::X_LEFT; }

	virtual bool IsEdgeValid(EDGE edge) const override;
	virtual edge_track_target GetEdgeConnectingPiece(EDGE edgeid) override;
	virtual unsigned int GetMaxConnectingPieces(EDGE direction) const override;

	virtual edge_track_target GetConnectingPieceByIndex(EDGE direction, unsigned int index) override {
		return GetConnectingPiece(direction);
	}

	virtual reservation_result ReservationV(const reservation_request_res &req) override;

	virtual std::string GetTypeName() const override { return "Crossover"; }

	static std::string GetTypeSerialisationNameStatic() { return "crossover"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	virtual unsigned int GetTRSList(std::vector<track_reservation_state *> &output_list) override {
		output_list.push_back(&trs);
		return 1;
	}

	protected:
	virtual EDGE GetAvailableAutoConnectionDirection(bool forward_connection) const override;
	virtual void GetListOfEdges(std::vector<edgelistitem> &output_list) const override;
};

#endif


