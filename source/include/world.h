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

struct connection_forward_declaration {
	generictrack *track1;
	DIRTYPE dir1;
	std::string name2;
	DIRTYPE dir2;
	connection_forward_declaration(generictrack *t1, DIRTYPE d1, const std::string &n2, DIRTYPE d2) : track1(t1), dir1(d1), name2(n2), dir2(d2) { }
};

class world_serialisation;

class world {
	friend world_serialisation;
	std::unordered_map<std::string, std::unique_ptr<generictrack> > all_pieces;
	std::deque<connection_forward_declaration> connection_forward_declarations;
	std::unordered_map<std::string, traction_type> traction_types;

	public:
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
};

#endif
