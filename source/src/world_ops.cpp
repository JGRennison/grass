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

#include "common.h"
#include "world_ops.h"
#include "util.h"
#include "serialisable_impl.h"

void future_usermessage::InitVariables(message_formatter &mf, world &w) {
	mf.RegisterVariable("gametime", [&](std::string) -> std::string {
		return w.FormatGameTime(w.GetGameTime());
	});
}

void future_usermessage::Deserialise(const deserialiser_input &di, error_collection &ec) {
	w = di.w;
}

void future_genericusermessage::PrepareVariables(message_formatter &mf, world &w) {
	
}

void future_genericusermessage::ExecuteAction() {
	world *w = GetWorld();
	if(w) {
		message_formatter mf;
		InitVariables(mf, *w);
		PrepareVariables(mf, *w);
		w->LogUserMessageLocal(LOG_MESSAGE, mf.FormatMessage(w->GetUserMessageTextpool().GetTextByName(textkey)));
	}
}

void future_genericusermessage::Deserialise(const deserialiser_input &di, error_collection &ec) {
	future_usermessage::Deserialise(di, ec);
	CheckTransJsonValue(textkey, di, "textkey", ec);
}

void future_genericusermessage::Serialise(serialiser_output &so, error_collection &ec) const {
	SerialiseValueJson(textkey, so, "textkey");
}
