//  grass - Generic Rail And Signalling Simulator
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
#include "util/util.h"
#include "core/world_ops.h"
#include "core/world_obj.h"
#include "core/serialisable_impl.h"
#include "core/text_pool.h"

void future_user_message::InitVariables(message_formatter &mf, world &w) {
	mf.RegisterVariable("gametime", [&](std::string) -> std::string {
		return w.FormatGameTime(w.GetGameTime());
	});
}

void future_user_message::Deserialise(const deserialiser_input &di, error_collection &ec) {
	future::Deserialise(di, ec);
	w = di.w;
}

void future_generic_user_message::PrepareVariables(message_formatter &mf, world &w) {

}

void future_generic_user_message::ExecuteAction() {
	world *w = GetWorld();
	if (w) {
		message_formatter mf;
		InitVariables(mf, *w);
		PrepareVariables(mf, *w);
		w->LogUserMessageLocal(LOG_MESSAGE, mf.FormatMessage(w->GetUserMessageTextpool().GetTextByName(text_key)));
	}
}

void future_generic_user_message::Deserialise(const deserialiser_input &di, error_collection &ec) {
	future_user_message::Deserialise(di, ec);
	CheckTransJsonValue(text_key, di, "text_key", ec);
}

void future_generic_user_message::Serialise(serialiser_output &so, error_collection &ec) const {
	future_user_message::Serialise(so, ec);
	SerialiseValueJson(text_key, so, "text_key");
}

void future_generic_user_message_reason::PrepareVariables(message_formatter &mf, world &w) {
	mf.RegisterVariable("reason", [&](const std::string &in) { return w.GetUserMessageTextpool().GetTextByName(this->reason_key); });

	world_obj *gt = dynamic_cast<world_obj*>(&GetTarget());
	if (gt) {
		mf.RegisterVariable("target", [=](const std::string &in) { return gt->GetName(); });
	}
}

void future_generic_user_message_reason::Deserialise(const deserialiser_input &di, error_collection &ec) {
	future_generic_user_message::Deserialise(di, ec);
	CheckTransJsonValue(reason_key, di, "reason_key", ec);
}

void future_generic_user_message_reason::Serialise(serialiser_output &so, error_collection &ec) const {
	future_generic_user_message::Serialise(so, ec);
	SerialiseValueJson(reason_key, so, "reason_key");
}
