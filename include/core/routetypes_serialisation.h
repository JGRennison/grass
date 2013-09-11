//  grass - Generic Rail And Signalling Simulator
//
//  WEBSITE: http://bitbucket.org/JGRennison/grass
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

#ifndef INC_ROUTETYPES_SERIALISATION_ALREADY
#define INC_ROUTETYPES_SERIALISATION_ALREADY

#include "core/routetypes.h"
#include "core/serialisable_impl.h"

namespace route_class {
	set Deserialise(const deserialiser_input &di, error_collection &ec);
	bool DeserialiseProp(const char *prop, set &value, const deserialiser_input &di, error_collection &ec);
	void DeserialiseGroup(set &s, const deserialiser_input &di, error_collection &ec, flag_conflict_checker<set> &conflictcheck);
	inline void DeserialiseGroup(set &s, const deserialiser_input &di, error_collection &ec) {
		flag_conflict_checker<set> conflictcheck;
		DeserialiseGroup(s, di, ec, conflictcheck);
	}
	void DeserialiseGroupProp(set &s, const deserialiser_input &di, const char *prop, error_collection &ec, flag_conflict_checker<set> &conflictcheck);
	inline void DeserialiseGroupProp(set &s, const deserialiser_input &di, const char *prop, error_collection &ec) {
		flag_conflict_checker<set> conflictcheck;
		DeserialiseGroupProp(s, di, prop, ec, conflictcheck);
	}
	std::pair<bool, ID> DeserialiseName(const std::string &name, error_collection &ec);
}

#endif
