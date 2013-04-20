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

#ifndef INC_TRACKCIRCUIT_ALREADY
#define INC_TRACKCIRCUIT_ALREADY

#include <string>
#include <vector>
#include "world_obj.h"
#include "flags.h"
#include "world.h"

class train;

struct train_ref {
	train *t;
	unsigned int count;
	train_ref(train *t_, unsigned int count_) : t(t_), count(count_) { }
};

class track_circuit : public world_obj {
	unsigned int traincount = 0;
	std::vector<train_ref> occupying_trains;
	std::vector<generictrack *> owned_pieces;
	world_time last_change;

	public:
	enum class TCF {
		ZERO		= 0,
		FORCEOCCUPIED	= 1<<0,
	};

	private:
	TCF tc_flags = TCF::ZERO;

	public:

	track_circuit(world &w_, const std::string &name_) : world_obj(w_), last_change(w_.GetGameTime()) { SetName(name_); }
	track_circuit(world &w_) : world_obj(w_), last_change(w_.GetGameTime()) { }
	void TrainEnter(train *t);
	void TrainLeave(train *t);
	inline bool Occupied() const;
	TCF GetTCFlags() const;
	TCF SetTCFlagsMasked(TCF bits, TCF mask);
	unsigned int GetTrainOccupationCount() const { return occupying_trains.size(); }
	world_time GetLastOccupationStateChangeTime() const { return last_change; }
	void RegisterTrack(generictrack *piece) { owned_pieces.push_back(piece); }
	const std::vector<generictrack *> &GetOwnedTrackSet() const { return owned_pieces; }

	virtual std::string GetTypeName() const override { return "Track Circuit"; }
	static std::string GetTypeSerialisationClassNameStatic() { return "trackcircuit"; }
	virtual std::string GetTypeSerialisationClassName() const override { return GetTypeSerialisationClassNameStatic(); }

	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};
template<> struct enum_traits< track_circuit::TCF > {	static constexpr bool flags = true; };

inline bool track_circuit::Occupied() const { return traincount > 0 || tc_flags & TCF::FORCEOCCUPIED; }

#endif
