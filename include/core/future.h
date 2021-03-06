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

#ifndef INC_FUTURE_ALREADY
#define INC_FUTURE_ALREADY

#include <list>
#include <memory>
#include <map>
#include <string>
#include "core/serialisable.h"
#include "common.h"

typedef uint64_t future_id_type;

class future_container;
class future_set;
class futurable_obj;
class serialisable_futurable_obj;

typedef deserialisation_type_factory<future_container &, serialisable_futurable_obj &, world_time, future_id_type> future_deserialisation_type_factory;

//all futures must be allocated with new
class future : public serialisable_obj {
	futurable_obj &target;
	world_time trigger_time;
	const uint64_t future_id = 1;
	future_set *registered_fs = 0;

	virtual void ExecuteAction() = 0;

	public:
	future(futurable_obj &targ, world_time ft, future_id_type id);
	virtual ~future();
	void Execute();
	virtual std::string GetTypeSerialisationName() const = 0;
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
	futurable_obj &GetTarget() const { return target; }
	world_time GetTriggerTime() const { return trigger_time; }
};

class future_container {
	public:
	virtual void RegisterFuture(const std::shared_ptr<future> &f) = 0;
	virtual void RemoveFuture(future &f) = 0;
	virtual void EnumerateRegisteredFutures(std::function<void(world_time, const std::shared_ptr<future> &)> func) const = 0;
};

class future_set : public future_container {
	std::multimap<world_time, std::shared_ptr<future> > futures;

	public:
	void ExecuteUpTo(world_time ft);
	virtual void RegisterFuture(const std::shared_ptr<future> &f) override;
	virtual void RemoveFuture(future &f) override;
	virtual void EnumerateRegisteredFutures(std::function<void(world_time, const std::shared_ptr<future> &)> func) const override;
};

class serialisable_futurable_obj;

class futurable_obj {
	//Use list because various parts of the code erase elements whilst iterating the list
	//vector iterator invalidation would break this
	//Don't use forward_list, because this reverses the order on serialisation round-trip
	std::list<future *> own_futures;

	public:
	virtual ~futurable_obj() { }
	void ClearFutures();
	void EnumerateFutures(std::function<void (future &)> f);
	void EnumerateFutures(std::function<void (const future &)> f) const;
	bool HaveFutures() const;

	private:
	friend future_set;
	void DeregisterFuture(future *f);
	void RegisterFuture(future *f);
};

class named_futurable_obj : public futurable_obj {
	public:
	virtual std::string GetTypeSerialisationClassName() const = 0;
	virtual std::string GetSerialisationName() const = 0;
	inline std::string GetFullSerialisationName() const {
		return GetTypeSerialisationClassName() + "/" + GetSerialisationName();
	}
};

class serialisable_futurable_obj : public serialisable_obj, public named_futurable_obj {
	public:
	void DeserialiseFutures(const deserialiser_input &di, error_collection &ec, const future_deserialisation_type_factory &dtf, future_container &fs);
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

template <typename C> void MakeFutureTypeWrapper(future_deserialisation_type_factory &future_types) {
	auto func = [&](const deserialiser_input &di, error_collection &ec, future_container &fc, serialisable_futurable_obj &sfo, world_time ft, future_id_type fid) {
		std::shared_ptr<C> f = std::make_shared<C>(sfo, ft, fid);
		f->DeserialiseObject(di, ec);
		fc.RegisterFuture(f);
	};
	future_types.RegisterType(C::GetTypeSerialisationNameStatic(), func);
}

#endif
