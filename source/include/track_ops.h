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

#include "track.h"
#include "future.h"
#include "serialisable.h"
#include "action.h"

class action_pointsaction;

class future_pointsaction : public future {
	friend action_pointsaction;

	unsigned int index;
	unsigned int bits;
	unsigned int mask;

	public:
	future_pointsaction(futurable_obj &targ, world_time ft, future_id_type id) : future(targ, ft, id) { };
	future_pointsaction(futurable_obj &targ, world_time ft, unsigned int index_, unsigned int bits_, unsigned int mask_) : future(targ, ft, 0), index(index_), bits(bits_), mask(mask_) { };
	static std::string GetTypeSerialisationNameStatic() { return "future_pointsaction"; }
	virtual std::string GetTypeSerialisationName() const { return GetTypeSerialisationNameStatic(); }
	virtual void ExecuteAction();
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec);
	virtual void Serialise(serialiser_output &so, error_collection &ec) const;
};

class action_pointsaction : public action {
	genericpoints *target;
	unsigned int index;
	unsigned int bits;
	unsigned int mask;
	
	private:
	void CancelFutures(unsigned int index, unsigned int setmask, unsigned int clearmask);
	world_time GetPointsMovementCompletionTime();

	public:
	action_pointsaction(world &w_) : action(w_), target(0) { }
	action_pointsaction(world &w_, genericpoints &targ, unsigned int index_, unsigned int bits_, unsigned int mask_) : action(w_), target(&targ), index(index_), bits(bits_), mask(mask_) { }
	static std::string GetTypeSerialisationNameStatic() { return "action_pointsaction"; }
	virtual std::string GetTypeSerialisationName() const { return GetTypeSerialisationNameStatic(); }
	virtual void ExecuteAction();
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec);
	virtual void Serialise(serialiser_output &so, error_collection &ec) const;
};
