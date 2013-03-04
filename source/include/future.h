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
#include "serialisable.h"
#include "common.h"

typedef uint64_t future_id_type;

class future_set;
class futurable_obj;
class serialisable_futurable_obj;

typedef deserialisation_type_factory<future_set &, serialisable_futurable_obj &, world_time, future_id_type> future_deserialisation_type_factory;

//all futures must be allocated with new
class future : public serialisable_obj {
	future_set &fs;
	futurable_obj &target;
	unsigned int f_flags;
	world_time trigger_time;
	const uint64_t future_id = 1;

	enum {
		FF_INFS		= 1<<0,
	};

	virtual void ExecuteAction() = 0;

	public:
	future(future_set &fs_, futurable_obj &targ, world_time ft, future_id_type id);
	virtual ~future();
	void Execute();
	void Cancel();
	virtual std::string GetTypeSerialisationName() const = 0;
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec);
	virtual void Serialise(serialiser_output &so, error_collection &ec) const;
	futurable_obj &GetTarget() const { return target; }
	future_set &GetFutureSet() const { return fs; }
	world_time GetTriggerTime() const { return trigger_time; }
	
	//only for internal use
	void MarkRemovedFromFutureSet() { f_flags &= ~FF_INFS; }
};

class future_set {
	std::multimap<world_time, future *> futures;
	uint64_t nextid;

	public:
	void RemoveFuture(const future &f);
	void ExecuteUpTo(world_time ft);
	
	private:
	friend future;
	uint64_t GetNextID() { return nextid++; }
	void RegisterFuture(future &f);
};

class serialisable_futurable_obj;

class futurable_obj {
	std::forward_list<std::unique_ptr<future> > own_futures;

	public:
	virtual ~futurable_obj() { }
	void ClearFutures();
	void EnumerateFutures(std::function<void (future &)> f);
	void EnumerateFutures(std::function<void (const future &)> f) const;
	bool HaveFutures() const;
	void RegisterFuture(std::unique_ptr<future> &&f);
	
	private:
	friend future;
	void ExterminateFuture(future *f);
};

class serialisable_futurable_obj : public serialisable_obj, public futurable_obj {

	public:
	void DeserialiseFutures(const deserialiser_input &di, error_collection &ec, const future_deserialisation_type_factory &dtf, future_set &fs);
	virtual void Serialise(serialiser_output &so, error_collection &ec) const;
};

#endif