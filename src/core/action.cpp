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

#include "common.h"
#include "core/action.h"

void action::Execute() const {
	ExecuteAction();
}

void action::ActionSendReplyFuture(const std::shared_ptr<future> &f) const {
	w.futures.RegisterFuture(f);
}

void action::ActionRegisterFuture(const std::shared_ptr<future> &f) const {
	w.futures.RegisterFuture(f);
}

void action::ActionRegisterLocalFuture(const std::shared_ptr<future> &f) const {
	w.futures.RegisterFuture(f);
}

void action::ActionCancelFuture(future &f) const {
	w.futures.RemoveFuture(f);
}

void action::ActionRegisterFutureAction(futurable_obj &targ, world_time ft, std::unique_ptr<action> &&a) const {
	ActionRegisterLocalFuture(std::make_shared<future_action_wrapper>(targ, ft, std::move(a)));
}

void future_action_wrapper::ExecuteAction() {
	if(act) act->Execute();
}

void future_action_wrapper::Deserialise(const deserialiser_input &di, error_collection &ec) {
	//TODO: fill this in
}

void future_action_wrapper::Serialise(serialiser_output &so, error_collection &ec) const {
	//TODO: fill this in
}
