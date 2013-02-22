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

class world_serialisation {
	world &w;

	public:
	world_serialisation(world &w_) : w(w_) { }
	void LoadGame(const rapidjson::Value &json, error_collection &ec);
	void DeserialiseObject(const deserialiser_input &di, error_collection &ec);
	template <typename T> T* MakeOrFindGenericTrack(const deserialiser_input &di, error_collection &ec);
	template <typename T> T* DeserialiseGenericTrack(const deserialiser_input &di, error_collection &ec);
};

#endif
