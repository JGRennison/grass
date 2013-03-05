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

#ifndef INC_FUTURE_ALREADY
#define INC_FUTURE_ALREADY

#include <forward_list>
#include <memory>
#include <map>
#include <string>
#include "serialisable.h"
#include "common.h"

typedef uint64_t future_id_type;

class future_container;
class future_set;
class futurable_obj;
class serialisable_futurable_obj;

typedef deserialisation_type_factory<future_container &, serialisable_futurable_obj &, world_time, future_id_type> future_deserialisation_type_factory;

//all futures must be allocated with new
class future : public serialisable_obj, public std::enable_shared_from_this<future> {
	futurable_obj &target;
	world_time trigger_time;
	const uint64_t future_id = 1;
	future_set *registered_fs = 0;
	
	static uint64_t lastid;

	virtual void ExecuteAction() = 0;

	public:
	future(futurable_obj &targ, world_time ft, future_id_type id);
	virtual ~future();
	void RegisterLocal(future_container &fs);
	void Execute();
	virtual std::string GetTypeSerialisationName() const = 0;
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec);
	virtual void Serialise(serialiser_output &so, error_collection &ec) const;
	futurable_obj &GetTarget() const { return target; }
	world_time GetTriggerTime() const { return trigger_time; }

	private:
	uint64_t MakeNewID() { return ++lastid; }
	public:
	static uint64_t &GetLastIDRef() { return lastid; }
};

class future_container {
	private:
	friend future;
	virtual void RegisterFuture(const std::shared_ptr<future> &f) = 0;
	virtual void RemoveFuture(future &f) = 0;
};

class future_set : public future_container {
	std::multimap<world_time, std::shared_ptr<future> > futures;

	public:
	void ExecuteUpTo(world_time ft);
	virtual void RegisterFuture(const std::shared_ptr<future> &f);
	virtual void RemoveFuture(future &f);
};

class serialisable_futurable_obj;

class futurable_obj {
	std::forward_list<future *> own_futures;

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
	virtual void Serialise(serialiser_output &so, error_collection &ec) const;
};

#endif