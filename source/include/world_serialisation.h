//  retcon
//
//  WEBSITE: http://retcon.sourceforge.net
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
//  2012 - j.g.rennison@gmail.com
//==========================================================================

#ifndef INC_WORLD_SERIALISATION_ALREADY
#define INC_WORLD_SERIALISATION_ALREADY

#include <map>
#include <string>

#include "world.h"
#include "serialisable.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
#include "rapidjson/document.h"
#pragma GCC diagnostic pop

class generictrack;

struct template_def {
	const rapidjson::Value *json = 0;
	bool beingexpanded = false;
};

class world_serialisation {
	world &w;
	std::map<std::string, template_def> template_map;
	std::forward_list<rapidjson::Document> parsed_inputs;
	generictrack *previoustrackpiece;

	public:
	deserialisation_type_factory<> object_types;

	void InitObjectTypes();
	world_serialisation(world &w_) : w(w_), previoustrackpiece(0) { InitObjectTypes(); }
	void ParseInputString(const std::string &input, error_collection &ec);
	void LoadGame(const deserialiser_input &di, error_collection &ec);
	void DeserialiseObject(const deserialiser_input &di, error_collection &ec);
	void DeserialiseTemplate(const deserialiser_input &di, error_collection &ec);
	void DeserialiseTypeDefinition(const deserialiser_input &di, error_collection &ec);
	void ExecuteTemplate(serialisable_obj &obj, std::string name, const deserialiser_input &di, error_collection &ec);
	void DeserialiseTractionType(const deserialiser_input &di, error_collection &ec);
	template <typename T> T* MakeOrFindGenericTrack(const deserialiser_input &di, error_collection &ec);
	template <typename T> T* DeserialiseGenericTrack(const deserialiser_input &di, error_collection &ec);
	template <typename C> void MakeGenericTrackTypeWrapper();
};

#endif
