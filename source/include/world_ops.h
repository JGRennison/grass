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

#ifndef INC_WORLDOPS_ALREADY
#define INC_WORLDOPS_ALREADY

#include "common.h"
#include "future.h"
#include "var.h"
#include "world.h"
#include "var.h"

class future_usermessage : public future {
	private:
	world *w;

	public:
	future_usermessage(futurable_obj &targ, world_time ft, future_id_type id) : future(targ, ft, id), w(0) {  };
	future_usermessage(futurable_obj &targ, world_time ft, world *w_) : future(targ, ft, 0), w(w_) {  };

	virtual void ExecuteAction() = 0;
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec);

	protected:
	world *GetWorld() const { return w; }
	void InitVariables(message_formatter &mf, world &w);
};

class future_genericusermessage : public future_usermessage {
	std::string textkey;

	public:
	future_genericusermessage(futurable_obj &targ, world_time ft, future_id_type id) : future_usermessage(targ, ft, id) { };
	future_genericusermessage(futurable_obj &targ, world_time ft, world *w_, const std::string &textkey_) : future_usermessage(targ, ft, w_), textkey(textkey_) { };
	static std::string GetTypeSerialisationNameStatic() { return "future_genericusermessage"; }
	virtual std::string GetTypeSerialisationName() const { return GetTypeSerialisationNameStatic(); }
	virtual void ExecuteAction() final;
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec);
	virtual void Serialise(serialiser_output &so, error_collection &ec) const;
	virtual void PrepareVariables(message_formatter &mf, world &w);
};

#endif
