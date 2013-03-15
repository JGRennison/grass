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

#include "serialisable.h"
#include "world.h"

#ifndef INC_ACTION_ALREADY
#define INC_ACTION_ALREADY

class action : public serialisable_obj {
	private:
	virtual void ExecuteAction() const = 0;

	protected:
	world &w;

	public:
	world_time action_time;

	action(world &w_) : w(w_), action_time(w_.GetGameTime()) { }
	virtual ~action() { }
	void Execute() const;
	void ActionSendReplyFuture(const std::shared_ptr<future> &f) const;
	void ActionRegisterFuture(const std::shared_ptr<future> &f) const;
	void ActionCancelFuture(future &f) const;
	virtual std::string GetTypeSerialisationName() const = 0;
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override = 0;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override = 0;
};

#endif