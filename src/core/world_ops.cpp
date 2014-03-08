//  grass - Generic Rail And Signalling Simulator
//
//  WEBSITE: http://bitbucket.org/JGRennison/grass
//  WEBSITE: http://grss.sourceforge.net
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version. See: COPYING-GPL.txt
//
//  This program  is distributed in the  hope that it will  be useful, but
//  WITHOUT   ANY  WARRANTY;   without  even   the  implied   warranty  of
//  MERCHANTABILITY  or FITNESS  FOR A  PARTICULAR PURPOSE.   See  the GNU
//  General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program. If not, see <http://www.gnu.org/licenses/>.
//
//  2013 - Jonathan Rennison <j.g.rennison@gmail.com>
//==========================================================================

#include "common.h"
#include "core/world_ops.h"
#include "core/world_obj.h"
#include "core/util.h"
#include "core/serialisable_impl.h"
#include "core/textpool.h"

void future_usermessage::InitVariables(message_formatter &mf, world &w) {
	mf.RegisterVariable("gametime", [&](std::string) -> std::string {
		return w.FormatGameTime(w.GetGameTime());
	});
}

void future_usermessage::Deserialise(const deserialiser_input &di, error_collection &ec) {
	future::Deserialise(di, ec);
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
	future_usermessage::Serialise(so, ec);
	SerialiseValueJson(textkey, so, "textkey");
}

void future_genericusermessage_reason::PrepareVariables(message_formatter &mf, world &w) {
	mf.RegisterVariable("reason", [&](const std::string &in) { return w.GetUserMessageTextpool().GetTextByName(this->reasonkey); });

	world_obj *gt = dynamic_cast<world_obj*>(&GetTarget());
	if(gt) mf.RegisterVariable("target", [=](const std::string &in) { return gt->GetName(); });
}

void future_genericusermessage_reason::Deserialise(const deserialiser_input &di, error_collection &ec) {
	future_genericusermessage::Deserialise(di, ec);
	CheckTransJsonValue(reasonkey, di, "reasonkey", ec);
}

void future_genericusermessage_reason::Serialise(serialiser_output &so, error_collection &ec) const {
	future_genericusermessage::Serialise(so, ec);
	SerialiseValueJson(reasonkey, so, "reasonkey");
}
