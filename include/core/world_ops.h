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

#ifndef INC_WORLDOPS_ALREADY
#define INC_WORLDOPS_ALREADY

#include "common.h"
#include "core/future.h"
#include "core/var.h"
#include "core/world.h"
#include "core/var.h"

class future_usermessage : public future {
	private:
	world *w;

	public:
	future_usermessage(futurable_obj &targ, world_time ft, future_id_type id) : future(targ, ft, id), w(0) {  };
	future_usermessage(futurable_obj &targ, world_time ft, world *w_) : future(targ, ft, w_->MakeNewFutureID()), w(w_) {  };

	virtual void ExecuteAction() = 0;
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;

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
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void ExecuteAction() override final;
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
	virtual void PrepareVariables(message_formatter &mf, world &w);
};

class future_genericusermessage_reason : public future_genericusermessage {
	std::string reasonkey;

	public:
	future_genericusermessage_reason(futurable_obj &targ, world_time ft, future_id_type id) : future_genericusermessage(targ, ft, id) { };
	future_genericusermessage_reason(futurable_obj &targ, world_time ft, world *w_, const std::string &textkey_, const std::string &reasonkey_) : future_genericusermessage(targ, ft, w_, textkey_), reasonkey(reasonkey_) { };
	static std::string GetTypeSerialisationNameStatic() { return "future_genericusermessage_reason"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void PrepareVariables(message_formatter &mf, world &w) override;
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

#endif
