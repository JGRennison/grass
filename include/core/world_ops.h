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

#ifndef INC_WORLDOPS_ALREADY
#define INC_WORLDOPS_ALREADY

#include "common.h"
#include "core/future.h"
#include "core/var.h"
#include "core/world.h"
#include "core/var.h"

class future_user_message : public future {
	private:
	world *w;

	public:
	future_user_message(futurable_obj &targ, world_time ft, future_id_type id)
			: future(targ, ft, id), w(nullptr) {  };
	future_user_message(futurable_obj &targ, world_time ft, world *w_)
			: future(targ, ft, w_->MakeNewFutureID()), w(w_) {  };

	virtual void ExecuteAction() = 0;
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;

	protected:
	world *GetWorld() const { return w; }
	void InitVariables(message_formatter &mf, world &w);
};

class future_generic_user_message : public future_user_message {
	std::string text_key;

	public:
	future_generic_user_message(futurable_obj &targ, world_time ft, future_id_type id)
			: future_user_message(targ, ft, id) { };
	future_generic_user_message(futurable_obj &targ, world_time ft, world *w_, const std::string &text_key_)
			: future_user_message(targ, ft, w_), text_key(text_key_) { };

	static std::string GetTypeSerialisationNameStatic() { return "future_generic_user_message"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void ExecuteAction() override final;
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
	virtual void PrepareVariables(message_formatter &mf, world &w);
};

class future_generic_user_message_reason : public future_generic_user_message {
	std::string reason_key;

	public:
	future_generic_user_message_reason(futurable_obj &targ, world_time ft, future_id_type id)
			: future_generic_user_message(targ, ft, id) { };
	future_generic_user_message_reason(futurable_obj &targ, world_time ft, world *w_, const std::string &text_key_, const std::string &reason_key_)
			: future_generic_user_message(targ, ft, w_, text_key_), reason_key(reason_key_) { };

	static std::string GetTypeSerialisationNameStatic() { return "future_generic_user_message_reason"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void PrepareVariables(message_formatter &mf, world &w) override;
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

#endif
