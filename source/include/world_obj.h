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

#ifndef INC_WORLDOBJ_ALREADY
#define INC_WORLDOBJ_ALREADY

#include "future.h"
#include "serialisable.h"

class world;

class world_obj : public serialisable_futurable_obj {
	std::string name;
	world &w;

	public:
	world_obj(world &w_) : w(w_) { }
	virtual std::string GetTypeName() const = 0;
	std::string GetName() const { return name; }
	virtual std::string GetSerialisationName() const { return GetName(); }
	void SetName(std::string newname) { name = newname; }
	virtual std::string GetFriendlyName() const;
	world &GetWorld() const { return w; }

	virtual void Deserialise(const deserialiser_input &di, error_collection &ec);
};

#endif