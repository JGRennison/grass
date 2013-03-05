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

#ifndef INC_WORLD_ALREADY
#define INC_WORLD_ALREADY

#include <unordered_map>
#include <deque>
#include "track.h"
#include "tractiontype.h"
#include "serialisable.h"
#include "future.h"

struct connection_forward_declaration {
	generictrack *track1;
	DIRTYPE dir1;
	std::string name2;
	DIRTYPE dir2;
	connection_forward_declaration(generictrack *t1, DIRTYPE d1, const std::string &n2, DIRTYPE d2) : track1(t1), dir1(d1), name2(n2), dir2(d2) { }
};

class world_serialisation;
class action;

typedef enum {
	GM_SINGLE,
	GM_SERVER,
	GM_CLIENT,
} GAMEMODE;

class world : public named_futurable_obj {
	friend world_serialisation;
	std::unordered_map<std::string, std::unique_ptr<generictrack> > all_pieces;
	std::deque<connection_forward_declaration> connection_forward_declarations;
	std::unordered_map<std::string, traction_type> traction_types;
	world_time gametime;
	GAMEMODE mode;

	public:
	future_deserialisation_type_factory future_types;
	future_set futures;

	void AddTrack(std::unique_ptr<generictrack> &&piece, error_collection &ec);
	inline void AddTractionType(std::string name, bool alwaysavailable) {
		traction_types[name].name=name;
		traction_types[name].alwaysavailable=alwaysavailable;
	}
	inline traction_type *GetTractionTypeByName(std::string name) {
		auto tt = traction_types.find(name);
		if(tt != traction_types.end()) return &(tt->second);
		else return 0;
	}
	void ConnectTrack(generictrack *track1, DIRTYPE dir1, std::string name2, DIRTYPE dir2, error_collection &ec);
	void ConnectAllPiecesInit(error_collection &ec);
	void PostLayoutInit(error_collection &ec);
	generictrack *FindTrackByName(const std::string &name) const;
	void InitFutureTypes();
	world() { InitFutureTypes(); }
	world_time GetGameTime() const { return gametime; }
	void SubmitAction(const action &request);
	named_futurable_obj *FindFuturableByName(const std::string &name) const;
	virtual std::string GetTypeSerialisationClassName() const { return ""; }
	virtual std::string GetSerialisationName() const { return "world"; }
};

template <typename C> void MakeFutureTypeWrapper(future_deserialisation_type_factory &future_types) {
	auto func = [&](const deserialiser_input &di, error_collection &ec, future_container &fc, serialisable_futurable_obj &sfo, world_time ft, future_id_type fid) {
		std::shared_ptr<C> f = std::make_shared<C>(sfo, ft, fid);
		f->DeserialiseObject(di, ec);
		f->RegisterLocal(fc);
	};
	future_types.RegisterType(C::GetTypeSerialisationNameStatic(), func);
}

#endif
